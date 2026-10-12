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
#include "../blitter/32bpp_anim.hpp"
#include "../blitter/32bpp_sse2.hpp"
#ifdef WITH_SSE
#include "../blitter/32bpp_anim_sse4.hpp"
#endif
#ifdef WITH_OPENGL
#include "../blitter/40bpp_anim.hpp"
#endif
#include "../gfx_func.h"
#include "../gfx_type.h"
#include "../palette_func.h"
#include "../cpu.h"
#include "../core/backup_type.hpp"
#include "../core/viewport_pixel_phase.hpp"
#include "../settings_type.h"
#include "../spritecache_type.h"
#include "../zoom_func.h"

#include "../safeguards.h"

/** Expose the reference sprite layout for synthetic sprites. */
class SmoothZoomTestBlitter : public Blitter_32bppSimple {
public:
	using Blitter_32bppSimple::Pixel;
};

/** Retain the per-pixel compositor as an independent reference for row batching. */
class SmoothZoomReferenceRowBlitter : public Blitter_32bppAnim {
protected:
	uint32_t DrawScaledNearestRow(Colour *destination, const Pixel *source, const int *columns,
			int first, int last, BlitterMode mode, const uint8_t *remap) override
	{
		uint32_t mask = 0;
		for (int x = first; x < last; ++x) {
			if (columns[x] < 0) continue;
			Pixel pixel = source[columns[x]];
			this->ResolveScaledPixel(pixel, mode, remap);
			if (pixel.a == 0) continue;
			this->WriteScaledPixel(destination + x, pixel, mode, remap);
			if (mode == BlitterMode::TransparentRemap) mask = std::numeric_limits<uint32_t>::max() >> (32 - PALETTE_ANIM_SIZE);
			else if (pixel.m >= PALETTE_ANIM_START && pixel.m < PALETTE_ANIM_START + PALETTE_ANIM_SIZE) mask |= uint32_t{1} << (pixel.m - PALETTE_ANIM_START);
		}
		return mask;
	}
};

TEST_CASE("Fractional pixel scrolling matches a complete sprite repaint", "[smoothzoom]")
{
	const float step = GENERATE(4.0f / 1.3f, 4.0f / 0.84f, 1.5f, 2.25f, 6.0f);
	SmoothZoomTestBlitter blitter;
	std::array<SmoothZoomTestBlitter::Pixel, 65 * 33> source;
	for (size_t i = 0; i < source.size(); ++i) {
		source[i] = {static_cast<uint8_t>(i * 17), static_cast<uint8_t>(i * 37), static_cast<uint8_t>(i * 61),
				static_cast<uint8_t>(i % 7 < 2 ? 0 : i % 7 == 2 ? 96 : 255), 0, 0};
	}
	constexpr int width = 8, height = 4;
	const Colour background(10, 20, 30);
	ViewportPixelPhase phase;
	phase.Reset(2, 3, step);
	auto render = [&]() {
		std::array<Colour, width * height> pixels;
		pixels.fill(background);
		Blitter::ScaledBlitterParams bp{source.data(), nullptr, 65, 33, phase.left, phase.top, step,
				width, height, pixels.data(), width};
		bp.nearest = true;
		REQUIRE(blitter.DrawScaled(&bp, BlitterMode::Normal));
		return pixels;
	};
	auto previous = render();
	for (int i = 1; i <= 24; ++i) {
		const auto [dx, dy] = phase.Move(2 + static_cast<int>(12 * std::sin(i * 0.17)),
				3 + static_cast<int>(5 * std::sin(i * 0.23)));
		auto complete = render();
		auto scrolled = complete; // Exposed strips are freshly rendered.
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				const int old_x = x - dx, old_y = y - dy;
				if (old_x >= 0 && old_x < width && old_y >= 0 && old_y < height) {
					scrolled[y * width + x] = previous[old_y * width + old_x];
				}
			}
		}
		CHECK(scrolled == complete);
		previous = complete;
	}
}

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

TEST_CASE("Binary viewport sampling agrees with floating-point centre coordinates", "[smoothzoom]")
{
	const double step = GENERATE(static_cast<double>(4.0f / 1.3f), static_cast<double>(4.0f / 0.84f),
			1.5, 6.0, static_cast<double>(15.99f), 1.3);
	const double displacement = GENERATE(0.0, -1e-9, 1e-9, -0.25, 0.125,
			-1.0 / (1U << 30), 1.0 / (1U << 30), -1.0 / (1U << 29), 1.0 / (1U << 29));
	const int clip_left = GENERATE(0, 1, 3);
	SmoothZoomTestBlitter blitter;
	std::array<SmoothZoomTestBlitter::Pixel, 65 * 33> source;
	for (size_t i = 0; i < source.size(); ++i) {
		source[i] = {static_cast<uint8_t>(i * 17), static_cast<uint8_t>(i * 37), static_cast<uint8_t>(i * 61), 255, 0, 0};
	}
	constexpr int width = 9, height = 6, pitch = 12;
	const Colour background(13, 29, 41);
	std::array<Colour, pitch * height> actual, expected;
	actual.fill(background);
	expected.fill(background);
	Blitter::ScaledBlitterParams bp{source.data(), nullptr, 65, 33, -step * 0.5 + displacement,
			0.25, step, width, height, actual.data(), pitch};
	bp.nearest = true;
	bp.clip_left = clip_left;
	bp.clip_top = 2;
	bp.clip_right = 3;
	bp.clip_bottom = 1;
	REQUIRE(blitter.DrawScaled(&bp, BlitterMode::Normal));
	const auto centre = [](double value) {
		const double boundary = std::floor(value * 32 + 0.5) / 32;
		return std::abs(value - boundary) <= 1e-9 ? boundary : value;
	};
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			const double sx = centre(bp.src_x + (x + 0.5) * step);
			const double sy = centre(bp.src_y + (y + 0.5) * step);
			if (sx < clip_left || sx >= 62 || sy < 2 || sy >= 32) continue;
			const auto &pixel = source[static_cast<int>(std::floor(sy)) * 65 + static_cast<int>(std::floor(sx))];
			expected[y * pitch + x] = Colour(pixel.r, pixel.g, pixel.b);
		}
	}
	CHECK(actual == expected);
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
	size_t capacity = 0;
protected:
	void *AllocatePtr(size_t size) override
	{
		if (size > this->capacity) {
			this->data = std::make_unique<uint8_t[]>(size);
			this->capacity = size;
		}
		return this->data.get();
	}
};

TEST_CASE("Batched fractional rows preserve pixels and native animation metadata", "[smoothzoom]")
{
	const BlitterMode mode = GENERATE(BlitterMode::Normal, BlitterMode::ColourRemap, BlitterMode::CrashRemap,
			BlitterMode::BlackRemap, BlitterMode::Transparent, BlitterMode::TransparentRemap);
	const bool animated_source = GENERATE(false, true);
	const bool palette_source = GENERATE(false, true);
	const bool white_source = GENERATE(false, true);
	const bool default_brightness = GENERATE(false, true);
	const int source_alpha = GENERATE(0, 1, 2, 3);
	CAPTURE(mode, animated_source, palette_source, white_source, default_brightness, source_alpha);
	AutoRestoreBackup animation_backup(_screen_disable_anim, false);
	AutoRestoreBackup screen_backup(_screen);
	AutoRestoreBackup palette_backup(_cur_palette);
	AutoRestoreBackup zoom_min_backup(_settings_client.gui.zoom_min, ZoomLevel::Min);
	AutoRestoreBackup zoom_max_backup(_settings_client.gui.zoom_max, ZoomLevel::Max);
	_cur_palette.palette[42] = Colour(30, 90, 160);
	_cur_palette.palette[47] = Colour(220, 40, 80);
	_cur_palette.palette[255] = Colour(255, 255, 255);
	_cur_palette.palette[PALETTE_ANIM_START + 2] = Colour(0, 240, 140);
	std::array<uint8_t, 256> remap;
	for (size_t i = 0; i < remap.size(); ++i) remap[i] = static_cast<uint8_t>(i);
	remap[42] = 0;
	remap[255] = PALETTE_ANIM_START + 2;
	remap[PALETTE_ANIM_START + 2] = 47;
	const uint8_t palette_index = white_source ? 255 : 42;
	SpriteLoader::SpriteCollection sprites;
	SpriteCollMap<std::vector<SpriteLoader::CommonPixel>> source;
	for (ZoomLevel zoom = ZoomLevel::Min; zoom <= ZoomLevel::Max; ++zoom) {
		const int width = UnScaleByZoom(65, zoom), height = UnScaleByZoom(33, zoom);
		auto &pixels = source[zoom];
		pixels.resize(width * height);
		for (size_t i = 0; i < pixels.size(); ++i) {
			const uint8_t index = !palette_source ? 0 : animated_source && i % 3 == 1 ? PALETTE_ANIM_START + 2 : i % 3 == 0 ? palette_index : 0;
			const bool palette_only = default_brightness && index != 0;
			pixels[i] = {static_cast<uint8_t>(palette_only ? 0 : i * 17), static_cast<uint8_t>(palette_only ? 0 : i * 37), static_cast<uint8_t>(palette_only ? 0 : i * 61),
					static_cast<uint8_t>(source_alpha == 0 ? 255 : source_alpha == 3 ? i : i % 7 < 2 ? 0 : source_alpha == 2 && i % 7 == 2 ? 96 : 255),
					index};
		}
		sprites[zoom] = {static_cast<uint16_t>(height), static_cast<uint16_t>(width), 0, 0, {}, pixels.data()};
	}
	constexpr int width = 25, pitch = 29, height = 8;
	_screen.width = width;
	_screen.height = height;
	_screen.pitch = pitch;
	std::vector<Colour> expected_pixels;
	std::vector<uint8_t> expected_buffer;
	uint32_t expected_mask = 0;
	SmoothZoomReferenceRowBlitter reference;
	Blitter_32bppAnim batched;
	std::vector<Blitter *> blitters{&reference, &batched};
#ifdef WITH_SSE
	Blitter_32bppSSE2_Anim sse2;
	Blitter_32bppSSE4_Anim sse4;
	blitters.push_back(&sse2);
	if (HasCPUIDFlag(1, 2, 19)) blitters.push_back(static_cast<Blitter_32bppSSE2_Anim *>(&sse4));
#endif
	for (Blitter *blitter : blitters) {
		CAPTURE(blitter->GetName());
		std::vector<Colour> pixels(pitch * height, Colour(10, 20, 30));
		_screen.dst_ptr = pixels.data();
		blitter->PostResize();
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) blitter->SetPixel(pixels.data(), x, y, PixelColour{PALETTE_ANIM_START + 2});
		}
		SmoothZoomSpriteAllocator allocator;
		const Sprite *sprite = blitter->Encode(SpriteType::Normal, sprites, allocator);
		Blitter::ScaledBlitterParams bp{sprite->data, remap.data(), sprite->width, sprite->height,
				-0.65, 0.1, 1.3, width, height, pixels.data(), pitch};
		bp.nearest = true;
		bp.clip_left = 1;
		REQUIRE(blitter->DrawScaled(&bp, mode));
		std::vector<uint8_t> buffer(blitter->BufferSize(width, height));
		blitter->CopyToBuffer(pixels.data(), buffer.data(), width, height);
		if (blitter == &reference) {
			expected_pixels = pixels;
			expected_buffer = buffer;
			expected_mask = bp.palette_mask;
		} else {
			if ((blitter->GetName() == "32bpp-sse4-anim" || blitter->GetName() == "32bpp-sse2-anim") && palette_source && !default_brightness && mode == BlitterMode::Normal) {
				/* The static-colour path reuses native SSE brightness rounding. */
				CHECK(std::equal(pixels.begin(), pixels.end(), expected_pixels.begin(), [](Colour a, Colour b) {
					return std::abs(int(a.r) - b.r) <= 1 && std::abs(int(a.g) - b.g) <= 1 && std::abs(int(a.b) - b.b) <= 1 && a.a == b.a;
				}));
			} else {
				CHECK(pixels == expected_pixels);
			}
			for (int y = 0; y < height; ++y) {
				const size_t offset = y * width * (sizeof(Colour) + sizeof(uint16_t)) + width * sizeof(Colour);
				CHECK(std::equal(buffer.begin() + offset, buffer.begin() + offset + width * sizeof(uint16_t), expected_buffer.begin() + offset));
			}
			CHECK(bp.palette_mask == expected_mask);
		}
	}
}

#ifdef WITH_SSE
TEST_CASE("Large fractional sources cannot wrap the animated cache budget", "[smoothzoom]")
{
	Blitter_32bppSSE_Base::SpriteData source{};
	source.infos[ZoomLevel::Min].sprite_width = 32768;
	Blitter_32bppSSE2 blitter;
	/* No output or source pixels are needed. The cache must reject this size
	 * before allocating colours or reading the absent encoded rows. */
	Blitter::ScaledBlitterParams bp{&source, nullptr, 32768, 16384, 0, 0, 1, 0, 0, nullptr, 0};
	bp.nearest = true;
	CHECK(blitter.DrawScaled(&bp, BlitterMode::Normal));
}

TEST_CASE("Fractional animated sprite reuse survives palette changes and eviction", "[smoothzoom]")
{
	AutoRestoreBackup animation_backup(_screen_disable_anim, false);
	AutoRestoreBackup screen_backup(_screen);
	AutoRestoreBackup palette_backup(_cur_palette);
	AutoRestoreBackup zoom_min_backup(_settings_client.gui.zoom_min, ZoomLevel::Min);
	AutoRestoreBackup zoom_max_backup(_settings_client.gui.zoom_max, ZoomLevel::Max);
	constexpr int width = 7, height = 5, pitch = 9;
	_screen.width = width;
	_screen.height = height;
	_screen.pitch = pitch;
	SpriteLoader::SpriteCollection sprites;
	SpriteCollMap<std::vector<SpriteLoader::CommonPixel>> source;
	for (ZoomLevel zoom = ZoomLevel::Min; zoom <= ZoomLevel::Max; ++zoom) {
		const int w = UnScaleByZoom(8, zoom), h = UnScaleByZoom(8, zoom);
		source[zoom].resize(w * h);
		sprites[zoom] = {static_cast<uint16_t>(h), static_cast<uint16_t>(w), 0, 0, {}, source[zoom].data()};
	}
	Blitter_32bppSSE2_Anim sse2;
	Blitter_32bppSSE4_Anim sse4;
	Blitter *sse = HasCPUIDFlag(1, 2, 19) ? static_cast<Blitter_32bppSSE2_Anim *>(&sse4) : &sse2;
	Blitter_32bppAnim reference;
	SmoothZoomSpriteAllocator allocator;
	SmoothZoomSpriteAllocator reference_allocator;
	SmoothZoomSpriteAllocator retained_allocator;
	SmoothZoomSpriteAllocator retained_reference_allocator;
	const Sprite *retained = nullptr;
	const Sprite *retained_reference = nullptr;
	const Sprite *previous = nullptr;
	auto compare = [&](const Sprite *sprite, const Sprite *reference_sprite) {
		std::vector<Colour> expected(pitch * height, Colour(10, 20, 30));
		std::vector<Colour> actual = expected;
		uint32_t expected_mask = 0;
		std::vector<uint8_t> expected_buffer;
		for (Blitter *blitter : std::array<Blitter *, 2>{&reference, sse}) {
			auto &pixels = blitter == &reference ? expected : actual;
			_screen.dst_ptr = pixels.data();
			blitter->PostResize();
			for (int y = 0; y < height; ++y) {
				for (int x = 0; x < width; ++x) blitter->SetPixel(pixels.data(), x, y, PixelColour{PALETTE_ANIM_START + 2});
			}
			const Sprite *encoded = blitter == &reference ? reference_sprite : sprite;
			Blitter::ScaledBlitterParams bp{encoded->data, nullptr, encoded->width, encoded->height,
					-0.65, 0.1, 1.3, width, height, pixels.data(), pitch};
			bp.nearest = true;
			REQUIRE(blitter->DrawScaled(&bp, BlitterMode::Normal));
			std::vector<uint8_t> buffer(blitter->BufferSize(width, height));
			blitter->CopyToBuffer(pixels.data(), buffer.data(), width, height);
			if (blitter == &reference) {
				expected_mask = bp.palette_mask;
				expected_buffer = buffer;
			} else {
				CHECK(actual == expected);
				CHECK(buffer == expected_buffer);
				CHECK(bp.palette_mask == expected_mask);
			}
		}
	};
	/* Re-encoding uses the same allocation. A retained sprite crosses the
	 * entry limit and must still draw correctly after being evicted. */
	for (int revision = 0; revision < 300; ++revision) {
		CAPTURE(revision);
		for (ZoomLevel zoom = ZoomLevel::Min; zoom <= ZoomLevel::Max; ++zoom) {
			auto &pixels = source[zoom];
			for (size_t i = 0; i < pixels.size(); ++i) {
				pixels[i] = {0, 0, 0, static_cast<uint8_t>((i + revision) % 3 == 0 ? 0 : (i + revision) % 3 == 1 ? 96 : 255),
						static_cast<uint8_t>(PALETTE_ANIM_START + 2 + revision % 2)};
			}
		}
		const Sprite *sprite = sse->Encode(SpriteType::Normal, sprites, allocator);
		const Sprite *reference_sprite = reference.Encode(SpriteType::Normal, sprites, reference_allocator);
		if (previous != nullptr) REQUIRE(sprite == previous);
		previous = sprite;
		if (revision == 0) {
			retained = sse->Encode(SpriteType::Normal, sprites, retained_allocator);
			retained_reference = reference.Encode(SpriteType::Normal, sprites, retained_reference_allocator);
			compare(retained, retained_reference);
		}
		for (int palette = 0; palette < 2; ++palette) {
			_cur_palette.palette[PALETTE_ANIM_START + 2] = palette == 0 ? Colour(200, 40, 80) : Colour(0, 240, 140);
			_cur_palette.palette[PALETTE_ANIM_START + 3] = palette == 0 ? Colour(80, 160, 240) : Colour(240, 80, 160);
			compare(sprite, reference_sprite);
			compare(sprite, reference_sprite);
		}
	}
	compare(retained, retained_reference);
}
#endif

TEST_CASE("Fractional encoded sprite decoding agrees with integer blitting at every mipmap", "[smoothzoom]")
{
	AutoRestoreBackup animation_backup(_screen_disable_anim, true);
	Colour screen_pixel;
	AutoRestoreBackup screen_backup(_screen.dst_ptr, static_cast<void *>(&screen_pixel));
	AutoRestoreBackup palette_backup(_cur_palette, _cur_palette);
	_cur_palette.palette[42] = Colour(30, 90, 160);
	_cur_palette.palette[PALETTE_ANIM_START + 2] = Colour(0, 240, 140);
	const bool nearest = GENERATE(false, true);
	const bool partial_alpha = GENERATE(false, true);
	const bool palette_pixels = GENERATE(false, true);
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
					static_cast<uint8_t>(i % 7 < 3 ? 0 : partial_alpha && i % 7 == 3 ? 96 : 255),
					static_cast<uint8_t>(!palette_pixels ? 0 : i % 11 == 3 ? 42 : i % 11 == 5 ? PALETTE_ANIM_START + 2 : 0)};
		}
		sprites[zoom] = {static_cast<uint16_t>(height), static_cast<uint16_t>(width), 0, 0, {}, pixels.data()};
	}
	std::vector<std::unique_ptr<Blitter>> blitters;
	blitters.push_back(std::make_unique<Blitter_32bppOptimized>());
#ifdef WITH_OPENGL
	blitters.push_back(std::make_unique<Blitter_40bppAnim>());
#endif
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
			if ((partial_alpha || palette_pixels) && blitter->GetName() == "32bpp-sse2") {
				/* Native SSE brightness and alpha arithmetic round differently
				 * from their scalar counterparts. Allow only one RGB unit for
				 * those cases. Opaque non-palette sprites remain byte-exact. */
				CHECK(std::equal(scaled.begin(), scaled.end(), reference.begin(), [](Colour a, Colour b) {
					return std::abs(int(a.r) - b.r) <= 1 && std::abs(int(a.g) - b.g) <= 1 &&
							std::abs(int(a.b) - b.b) <= 1 && a.a == b.a;
				}));
			} else {
				CHECK(scaled == reference);
			}
		}
	}
}

TEST_CASE("Unaligned encoded sub-sprite edges use a mipmap that preserves the clipped source", "[smoothzoom]")
{
	AutoRestoreBackup animation_backup(_screen_disable_anim, true);
	Colour screen_pixel;
	AutoRestoreBackup screen_backup(_screen.dst_ptr, static_cast<void *>(&screen_pixel));
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
#ifdef WITH_OPENGL
	blitters.push_back(std::make_unique<Blitter_40bppAnim>());
#endif
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
