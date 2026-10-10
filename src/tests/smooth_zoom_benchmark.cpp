/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file smooth_zoom_benchmark.cpp Optional production-blitter timing experiment. */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../blitter/32bpp_optimized.hpp"
#include "../blitter/32bpp_anim.hpp"
#include "../blitter/32bpp_sse2.hpp"
#include "../core/backup_type.hpp"
#include "../settings_type.h"
#include "../spritecache_type.h"
#include "../zoom_func.h"

#include <chrono>
#include <iostream>

#include "../safeguards.h"

/** Keep an encoded sprite alive for the duration of one timing experiment. */
class SmoothZoomBenchmarkAllocator : public SpriteAllocator {
	std::unique_ptr<uint8_t[]> data;
protected:
	void *AllocatePtr(size_t size) override
	{
		this->data = std::make_unique<uint8_t[]>(size);
		return this->data.get();
	}
};

/**
 * Run explicitly with openttd_test "[smoothzoom-benchmark]". This hidden case is
 * excluded from normal correctness tests. Timings include row decoding,
 * filtering, palette resolution and destination composition, but exclude sprite
 * encoding and viewport scene construction. They are not an end-to-end FPS test.
 */
TEST_CASE("Measure integer and fractional production blitter throughput", "[.][smoothzoom-benchmark]")
{
	AutoRestoreBackup zoom_min_backup(_settings_client.gui.zoom_min, ZoomLevel::Min);
	AutoRestoreBackup zoom_max_backup(_settings_client.gui.zoom_max, ZoomLevel::Max);
	AutoRestoreBackup disable_anim_backup(_screen_disable_anim, true);
	AutoRestoreBackup palette_backup(_cur_palette);
	for (int i = 0; i < PALETTE_ANIM_SIZE; i++) {
		_cur_palette.palette[PALETTE_ANIM_START + i] = Colour(20 + i, 60 + i, 120 + i);
	}
	std::vector<std::unique_ptr<Blitter>> blitters;
	blitters.push_back(std::make_unique<Blitter_32bppOptimized>());
	blitters.push_back(std::make_unique<Blitter_32bppAnim>());
#ifdef WITH_SSE
	blitters.push_back(std::make_unique<Blitter_32bppSSE2>());
#endif
	std::cout << "blitter,source,method,step,draws,us_per_draw,us_per_destination_megapixel,checksum\n";
	for (auto &blitter : blitters) {
		for (bool terrain : {false, true}) {
			SpriteLoader::SpriteCollection sprites;
			SpriteCollMap<std::vector<SpriteLoader::CommonPixel>> source;
			for (ZoomLevel zoom = ZoomLevel::Min; zoom <= ZoomLevel::Max; zoom++) {
				const int factor = ScaleByZoom(1, zoom);
				const int width = UnScaleByZoom(256, zoom);
				const int height = UnScaleByZoom(128, zoom);
				auto &pixels = source[zoom];
				pixels.resize(width * height);
				for (int y = 0; y < height; y++) {
					for (int x = 0; x < width; x++) {
						const double radius = std::abs((x + 0.5) * factor - 128) / 128 +
								std::abs((y + 0.5) * factor - 64) / 64;
						const uint8_t alpha = !terrain || radius < 0.9 ? 255 : radius <= 1 ? 128 : 0;
						pixels[y * width + x] = terrain ? SpriteLoader::CommonPixel{
								static_cast<uint8_t>(40 + x * factor / 2), static_cast<uint8_t>(80 + y * factor),
								static_cast<uint8_t>(60 + (x + y) % 120), alpha, 0} :
								SpriteLoader::CommonPixel{0, 0, 0, alpha,
								static_cast<uint8_t>(PALETTE_ANIM_START + (x + y) % PALETTE_ANIM_SIZE)};
					}
				}
				sprites[zoom] = {static_cast<uint16_t>(height), static_cast<uint16_t>(width), 0, 0, {}, pixels.data()};
			}
			SmoothZoomBenchmarkAllocator allocator;
			const Sprite *sprite = blitter->Encode(SpriteType::Normal, sprites, allocator);
			for (ZoomLevel zoom : {ZoomLevel::Normal, ZoomLevel::Out4x}) {
				for (int sampler : {0, 1, 2}) {
					const bool fractional = sampler != 0;
					const double step = fractional ? (zoom == ZoomLevel::Normal ? 4.0 / 1.3 : 20.0) : ScaleByZoom(1, zoom);
					const int width = static_cast<int>(std::ceil(256 / step));
					const int height = static_cast<int>(std::ceil(128 / step));
					std::vector<Colour> output((width + 1) * height, Colour(10, 20, 30));
					Blitter::BlitterParams integer{sprite->data, nullptr, 0, 0, width, height, sprite->width, sprite->height,
							0, 0, output.data(), width + 1};
					Blitter::ScaledBlitterParams scaled{sprite->data, nullptr, sprite->width, sprite->height,
							0.35, 0.2, step, width, height, output.data(), width + 1};
					scaled.nearest = sampler == 2;
					/* Warm up scratch buffers before starting the clock. */
					if (fractional) REQUIRE(blitter->DrawScaled(&scaled, BlitterMode::Normal));
					else blitter->Draw(&integer, BlitterMode::Normal, zoom);
					uint64_t draws = 0;
					uint64_t checksum = 0;
					const auto start = std::chrono::steady_clock::now();
					auto finish = start;
					do {
						for (int i = 0; i < 64; i++) {
							if (fractional) blitter->DrawScaled(&scaled, BlitterMode::Normal);
							else blitter->Draw(&integer, BlitterMode::Normal, zoom);
							checksum += output[draws % output.size()].data;
							draws++;
						}
						finish = std::chrono::steady_clock::now();
					} while (finish - start < std::chrono::milliseconds(120));
					const double elapsed_us = std::chrono::duration<double, std::micro>(finish - start).count();
					const double us_per_draw = elapsed_us / draws;
					std::cout << fmt::format("{},{},{},{:.6f},{},{:.3f},{:.3f},{}\n", blitter->GetName(),
							terrain ? "terrain-alpha" : "opaque-palette", sampler == 2 ? "motion" : fractional ? "fractional" : "integer", step, draws,
							us_per_draw, us_per_draw * 1000000 / (width * height), checksum);
					CHECK(draws > 0);
					CHECK(checksum > 0);
				}
			}
		}
	}
}
