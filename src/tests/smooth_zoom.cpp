/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file smooth_zoom.cpp Tests for fractional sprite filtering and clipping. */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../blitter/32bpp_simple.hpp"
#include "../blitter/32bpp_optimized.hpp"
#include "../blitter/32bpp_sse2.hpp"
#include "../gfx_type.h"
#include "../palette_func.h"
#include "../core/backup_type.hpp"
#include "../settings_type.h"
#include "../spritecache_type.h"
#include "../zoom_func.h"

#include "../safeguards.h"

/** Expose the reference sprite layout for synthetic sprites. */
class SmoothZoomTestBlitter : public Blitter_32bppSimple {
public:
	using Blitter_32bppSimple::Pixel;
};

TEST_CASE("Motion sampling selects pixel centres without touching pitch guards", "[smoothzoom]")
{
	SmoothZoomTestBlitter blitter;
	std::array<SmoothZoomTestBlitter::Pixel, 6> sprite{{
		{200, 0, 0, 255, 0, 0}, {0, 200, 0, 255, 0, 0}, {0, 0, 200, 255, 0, 0},
		{200, 0, 0, 255, 0, 0}, {0, 200, 0, 255, 0, 0}, {0, 0, 200, 255, 0, 0},
	}};
	const Colour sentinel(11, 22, 33);
	std::array<Colour, 4> output{sentinel, sentinel, sentinel, sentinel};
	Blitter::ScaledBlitterParams bp{sprite.data(), nullptr, 3, 2, 0, 0, 1.5, 2, 1, output.data(), 4};
	bp.nearest = true;
	REQUIRE(blitter.DrawScaled(&bp, BlitterMode::Normal));
	CHECK(output[0] == Colour(200, 0, 0));
	CHECK(output[1] == Colour(0, 0, 200));
	CHECK(output[2] == sentinel);
	CHECK(output[3] == sentinel);
	bp.clip_left = 1;
	output.fill(sentinel);
	REQUIRE(blitter.DrawScaled(&bp, BlitterMode::Normal));
	CHECK(output[0] == sentinel);
	CHECK(output[1] == Colour(0, 0, 200));
}

TEST_CASE("Motion palette dependencies follow the sampled remapped pixel", "[smoothzoom]")
{
	AutoRestoreBackup palette_backup(_cur_palette);
	SmoothZoomTestBlitter blitter;
	const uint8_t animated = PALETTE_ANIM_START;
	_cur_palette.palette[animated] = Colour(20, 40, 60);
	std::array<SmoothZoomTestBlitter::Pixel, 4> sprite{{
		{0, 0, 0, 255, animated, DEFAULT_BRIGHTNESS}, {80, 90, 100, 255, 0, 0},
		{80, 90, 100, 255, 0, 0}, {0, 0, 0, 255, 10, DEFAULT_BRIGHTNESS},
	}};
	std::array<uint8_t, 256> remap{};
	remap[10] = animated + 1;
	Colour output;
	Blitter::ScaledBlitterParams bp{sprite.data(), remap.data(), 2, 2, 0, 0, 2, 1, 1, &output, 1};
	bp.nearest = true;
	REQUIRE(blitter.DrawScaled(&bp, BlitterMode::Normal));
	CHECK(bp.palette_mask == 0); // Off-centre animated colour does not contribute.
	REQUIRE(blitter.DrawScaled(&bp, BlitterMode::ColourRemap));
	CHECK(bp.palette_mask == 2);
	remap[10] = 0;
	REQUIRE(blitter.DrawScaled(&bp, BlitterMode::ColourRemap));
	CHECK(bp.palette_mask == 0);
}

TEST_CASE("Filtered animation dependencies include off-centre and remapped samples", "[smoothzoom]")
{
	AutoRestoreBackup palette_backup(_cur_palette);
	SmoothZoomTestBlitter blitter;
	const uint8_t animated = PALETTE_ANIM_START;
	_cur_palette.palette[animated] = Colour(20, 40, 60);
	std::array<SmoothZoomTestBlitter::Pixel, 4> sprite{{
		{0, 0, 0, 255, animated, DEFAULT_BRIGHTNESS}, {80, 90, 100, 255, 0, 0},
		{80, 90, 100, 255, 0, 0}, {80, 90, 100, 255, 0, 0},
	}};
	Colour output;
	Blitter::ScaledBlitterParams bp{sprite.data(), nullptr, 2, 2, 0, 0, 2, 1, 1, &output, 1};
	REQUIRE(blitter.DrawScaled(&bp, BlitterMode::Normal));
	CHECK(bp.palette_mask == 1);
	/* The centre is opaque; an invisible off-centre animated tap adds no dependency. */
	sprite[0].a = 0;
	REQUIRE(blitter.DrawScaled(&bp, BlitterMode::Normal));
	CHECK(bp.palette_mask == 0);

	sprite[0] = {0, 0, 0, 255, 10, DEFAULT_BRIGHTNESS};
	std::array<uint8_t, 256> remap{};
	remap[10] = animated + 1;
	bp.remap = remap.data();
	REQUIRE(blitter.DrawScaled(&bp, BlitterMode::ColourRemap));
	CHECK(bp.palette_mask == 2);
	remap[10] = 0;
	REQUIRE(blitter.DrawScaled(&bp, BlitterMode::ColourRemap));
	CHECK(bp.palette_mask == 0);
	/* Alpha masks don't consume source palette colours. */
	sprite[0] = {0, 0, 0, 255, animated, DEFAULT_BRIGHTNESS};
	REQUIRE(blitter.DrawScaled(&bp, BlitterMode::BlackRemap));
	CHECK(bp.palette_mask == 0);
}

TEST_CASE("Fractional sprite filtering averages source coverage and respects destination pitch", "[smoothzoom]")
{
	SmoothZoomTestBlitter blitter;
	std::array<SmoothZoomTestBlitter::Pixel, 9> sprite{};
	for (size_t i = 0; i < sprite.size(); i++) sprite[i] = {static_cast<uint8_t>((i % 3) * 120), 60, 90, 255, 0, 0};
	const Colour sentinel(11, 22, 33);
	std::array<Colour, 8> output;
	output.fill(sentinel);
	Blitter::ScaledBlitterParams bp{sprite.data(), nullptr, 3, 3, 0, 0, 1.5, 2, 2, output.data(), 4};
	REQUIRE(blitter.DrawScaled(&bp, BlitterMode::Normal));
	for (int y = 0; y < 2; y++) {
		CHECK(output[y * 4].r == 40);
		CHECK(output[y * 4 + 1].r == 200);
		CHECK(output[y * 4].g == 60);
		CHECK(output[y * 4 + 1].b == 90);
		CHECK(output[y * 4 + 2] == sentinel);
		CHECK(output[y * 4 + 3] == sentinel);
	}
}

TEST_CASE("Fractional sub-sprite clipping excludes colour outside the selected region", "[smoothzoom]")
{
	const bool nearest = GENERATE(false, true);
	SmoothZoomTestBlitter blitter;
	std::array<SmoothZoomTestBlitter::Pixel, 9> sprite{};
	for (size_t i = 0; i < sprite.size(); i++) sprite[i] = i % 3 == 0 ?
			SmoothZoomTestBlitter::Pixel{255, 0, 0, 255, 0, 0} : SmoothZoomTestBlitter::Pixel{0, 255, 0, 255, 0, 0};
	const Colour sentinel(11, 22, 33);
	std::array<Colour, 2> output{sentinel, sentinel};
	Blitter::ScaledBlitterParams bp{sprite.data(), nullptr, 3, 3, 0, 0, 1.5, 2, 1, output.data(), 2};
	bp.clip_left = 1;
	bp.nearest = nearest;
	REQUIRE(blitter.DrawScaled(&bp, BlitterMode::Normal));
	CHECK(output[0] == sentinel);
	CHECK(output[1] == Colour(0, 255, 0));
}

TEST_CASE("Adjoining opaque sprites do not expose the destination through filtered edges", "[smoothzoom]")
{
	const bool nearest = GENERATE(false, true);
	SmoothZoomTestBlitter blitter;
	std::array<SmoothZoomTestBlitter::Pixel, 6> sprite{};
	for (auto &pixel : sprite) pixel = {200, 100, 50, 255, 0, 0};
	Colour output(0, 0, 0);
	Blitter::ScaledBlitterParams bp{sprite.data(), nullptr, 2, 3, 1.5, 0, 1, 1, 1, &output, 1};
	bp.nearest = nearest;
	REQUIRE(blitter.DrawScaled(&bp, BlitterMode::Normal));
	/* The centre belongs to the adjoining sprite, whose left edge covers the other half. */
	bp.src_x = -0.5;
	REQUIRE(blitter.DrawScaled(&bp, BlitterMode::Normal));
	CHECK(output == Colour(200, 100, 50));
}

TEST_CASE("Fractional rendering is independent of dirty rectangle partitioning", "[smoothzoom]")
{
	const bool nearest = GENERATE(false, true);
	SmoothZoomTestBlitter blitter;
	std::array<SmoothZoomTestBlitter::Pixel, 256> sprite{};
	for (size_t i = 0; i < sprite.size(); i++) {
		sprite[i] = {static_cast<uint8_t>(i * 17), static_cast<uint8_t>(i * 37), static_cast<uint8_t>(i * 61),
				static_cast<uint8_t>(i % 7 == 0 ? 0 : 255), 0, 0};
	}
	std::array<Colour, 96> full;
	const double origin = GENERATE(-5.15, -0.35, 0.0, 0.7, 3.25);
	const double step = GENERATE(1.01, 1.7, 1.9999, 2.0001, 3.9999, 4.0001, static_cast<double>(4.0f / 1.3f), static_cast<double>(4.0f / 0.84f));
	CAPTURE(origin, step);
	full.fill(Colour(20, 30, 40));
	auto split = full;
	Blitter::ScaledBlitterParams bp{sprite.data(), nullptr, 16, 16, origin, origin - 0.2, step, 8, 8, full.data(), 12};
	bp.nearest = nearest;
	REQUIRE(blitter.DrawScaled(&bp, BlitterMode::Normal));
	for (int y = 0; y < 8; y += 2) {
		for (int x = 0; x < 8; x += 2) {
			auto part = bp;
			part.src_x += x * bp.step;
			part.src_y += y * bp.step;
			part.width = part.height = 2;
			part.dst = split.data() + y * bp.pitch + x;
			REQUIRE(blitter.DrawScaled(&part, BlitterMode::Normal));
		}
	}
	for (size_t i = 0; i < full.size(); i++) {
		CAPTURE(i, full[i].data, split[i].data);
		CHECK(full[i] == split[i]);
	}
}

TEST_CASE("Fractional source translucency blends with the destination", "[smoothzoom]")
{
	const bool nearest = GENERATE(false, true);
	SmoothZoomTestBlitter blitter;
	std::array<SmoothZoomTestBlitter::Pixel, 9> sprite{};
	for (auto &pixel : sprite) pixel = {200, 120, 40, 128, 0, 0};
	Colour output(100, 80, 60);
	Blitter::ScaledBlitterParams bp{sprite.data(), nullptr, 3, 3, 0, 0, 1.5, 1, 1, &output, 1};
	bp.nearest = nearest;
	REQUIRE(blitter.DrawScaled(&bp, BlitterMode::Normal));
	CHECK(output == Colour(150, 100, 50));
}

TEST_CASE("Fractional opaque pixel effects retain integer blitter semantics", "[smoothzoom]")
{
	const bool nearest = GENERATE(false, true);
	AutoRestoreBackup palette_backup(_cur_palette);
	_cur_palette.palette[42] = Colour(200, 150, 100);
	SmoothZoomTestBlitter blitter;
	SmoothZoomTestBlitter::Pixel pixel{240, 180, 60, 255, 0, 0};
	std::array<uint8_t, 256> remap;
	remap.fill(42);
	for (BlitterMode mode : {BlitterMode::Normal, BlitterMode::ColourRemap, BlitterMode::CrashRemap,
			BlitterMode::BlackRemap, BlitterMode::Transparent, BlitterMode::TransparentRemap}) {
		CAPTURE(mode);
		Colour integer(128, 80, 40);
		Colour scaled = integer;
		Blitter::BlitterParams reference{&pixel, remap.data(), 0, 0, 1, 1, 1, 1, 0, 0, &integer, 1};
		blitter.Draw(&reference, mode, ZoomLevel::Min);
		Blitter::ScaledBlitterParams fractional{&pixel, remap.data(), 1, 1, 0, 0, 1, 1, 1, &scaled, 1};
		fractional.nearest = nearest;
		REQUIRE(blitter.DrawScaled(&fractional, mode));
		CHECK(scaled == integer);
	}
}

TEST_CASE("Fractional filtering resolves each company colour before averaging", "[smoothzoom]")
{
	AutoRestoreBackup palette_backup(_cur_palette);
	_cur_palette.palette[42] = Colour(240, 0, 0);
	_cur_palette.palette[43] = Colour(0, 0, 240);
	SmoothZoomTestBlitter blitter;
	std::array<SmoothZoomTestBlitter::Pixel, 4> sprite{{
		{0, 0, 0, 255, 10, DEFAULT_BRIGHTNESS}, {0, 0, 0, 255, 11, DEFAULT_BRIGHTNESS},
		{0, 0, 0, 255, 10, DEFAULT_BRIGHTNESS}, {0, 0, 0, 255, 11, DEFAULT_BRIGHTNESS},
	}};
	std::array<uint8_t, 256> remap{};
	remap[10] = 42;
	remap[11] = 43;
	Colour output(0, 0, 0);
	Blitter::ScaledBlitterParams bp{sprite.data(), remap.data(), 2, 2, 0, 0, 2, 1, 1, &output, 1};
	REQUIRE(blitter.DrawScaled(&bp, BlitterMode::ColourRemap));
	CHECK(output == Colour(120, 0, 120));
	SECTION("A company colour remapped to zero leaves its centre transparent") {
		remap[11] = 0;
		output = Colour(10, 20, 30);
		REQUIRE(blitter.DrawScaled(&bp, BlitterMode::ColourRemap));
		CHECK(output == Colour(10, 20, 30));
	}
}

/** Own one encoded test sprite without involving the global sprite cache. */
class SmoothZoomSpriteAllocator : public SpriteAllocator {
	std::unique_ptr<uint8_t[]> data;
protected:
	void *AllocatePtr(size_t size) override
	{
		this->data = std::make_unique<uint8_t[]>(size);
		return this->data.get();
	}
};

TEST_CASE("Fractional encoded sprite decoding agrees with integer blitting at every mipmap", "[smoothzoom]")
{
	const bool nearest = GENERATE(false, true);
	AutoRestoreBackup zoom_min_backup(_settings_client.gui.zoom_min, ZoomLevel::Min);
	AutoRestoreBackup zoom_max_backup(_settings_client.gui.zoom_max, ZoomLevel::Max);
	SpriteLoader::SpriteCollection sprites;
	SpriteCollMap<std::vector<SpriteLoader::CommonPixel>> source;
	for (ZoomLevel zoom = ZoomLevel::Min; zoom <= ZoomLevel::Max; zoom++) {
		int width = UnScaleByZoom(65, zoom);
		int height = UnScaleByZoom(33, zoom);
		auto &pixels = source[zoom];
		pixels.resize(width * height);
		for (size_t i = 0; i < pixels.size(); i++) {
			pixels[i] = {static_cast<uint8_t>(i * 17), static_cast<uint8_t>(i * 37), static_cast<uint8_t>(i * 61),
					static_cast<uint8_t>(i % 7 < 3 ? 0 : 255), 0};
		}
		sprites[zoom] = {static_cast<uint16_t>(height), static_cast<uint16_t>(width), 0, 0, {}, pixels.data()};
	}
	std::vector<std::unique_ptr<Blitter>> blitters;
	blitters.push_back(std::make_unique<Blitter_32bppOptimized>());
#ifdef WITH_SSE
	blitters.push_back(std::make_unique<Blitter_32bppSSE2>());
#endif
	for (auto &blitter : blitters) {
		CAPTURE(blitter->GetName());
		SmoothZoomSpriteAllocator allocator;
		const Sprite *sprite = blitter->Encode(SpriteType::Normal, sprites, allocator);
		for (ZoomLevel zoom = ZoomLevel::Min; zoom <= ZoomLevel::Max; zoom++) {
			CAPTURE(zoom);
			int width = UnScaleByZoom(sprite->width, zoom);
			int height = UnScaleByZoom(sprite->height, zoom);
			std::vector<Colour> reference((width + 1) * height, Colour(10, 20, 30));
			auto scaled = reference;
			Blitter::BlitterParams integer{sprite->data, nullptr, 0, 0, width, height, sprite->width, sprite->height,
					0, 0, reference.data(), width + 1};
			blitter->Draw(&integer, BlitterMode::Normal, zoom);
			Blitter::ScaledBlitterParams fractional{sprite->data, nullptr, sprite->width, sprite->height, 0, 0,
					static_cast<double>(ScaleByZoom(1, zoom)), width, height, scaled.data(), width + 1};
			fractional.nearest = nearest;
			REQUIRE(blitter->DrawScaled(&fractional, BlitterMode::Normal));
			CHECK(scaled == reference);
		}
	}
}

TEST_CASE("Unaligned encoded sub-sprite edges use a mipmap that preserves the clipped source", "[smoothzoom]")
{
	const bool nearest = GENERATE(false, true);
	AutoRestoreBackup zoom_min_backup(_settings_client.gui.zoom_min, ZoomLevel::Min);
	AutoRestoreBackup zoom_max_backup(_settings_client.gui.zoom_max, ZoomLevel::Max);
	SpriteLoader::SpriteCollection sprites;
	SpriteCollMap<std::vector<SpriteLoader::CommonPixel>> source;
	for (ZoomLevel zoom = ZoomLevel::Min; zoom <= ZoomLevel::Max; zoom++) {
		int width = UnScaleByZoom(8, zoom);
		int height = UnScaleByZoom(8, zoom);
		auto &pixels = source[zoom];
		pixels.resize(width * height);
		for (size_t i = 0; i < pixels.size(); i++) {
			/* Coarse source intentionally contains only the excluded colour. */
			pixels[i] = zoom == ZoomLevel::Min && i % width != 0 ?
					SpriteLoader::CommonPixel{0, 255, 0, 255, 0} : SpriteLoader::CommonPixel{255, 0, 0, 255, 0};
		}
		sprites[zoom] = {static_cast<uint16_t>(height), static_cast<uint16_t>(width), 0, 0, {}, pixels.data()};
	}
	std::vector<std::unique_ptr<Blitter>> blitters;
	blitters.push_back(std::make_unique<Blitter_32bppOptimized>());
#ifdef WITH_SSE
	blitters.push_back(std::make_unique<Blitter_32bppSSE2>());
#endif
	for (auto &blitter : blitters) {
		CAPTURE(blitter->GetName());
		SmoothZoomSpriteAllocator allocator;
		const Sprite *sprite = blitter->Encode(SpriteType::Normal, sprites, allocator);
		for (double step : {1.9999, 2.0001, 3.9999, 4.0001}) {
			CAPTURE(step);
			Colour output(10, 20, 30);
			Blitter::ScaledBlitterParams bp{sprite->data, nullptr, 8, 8, 0.25, 0.1, step, 1, 1, &output, 1};
			bp.clip_left = 1;
			bp.nearest = nearest;
			REQUIRE(blitter->DrawScaled(&bp, BlitterMode::Normal));
			CHECK(output == Colour(0, 255, 0));
		}
	}
}
