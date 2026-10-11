/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file 32bpp_sse2.cpp Implementation of the SSE2 32 bpp blitter. */

#ifdef WITH_SSE

#include "../stdafx.h"
#include "../zoom_func.h"
#include "../settings_type.h"
#include "32bpp_sse2.hpp"
#include "32bpp_sse_func.hpp"
#include <atomic>

#include "../safeguards.h"

/** Instantiation of the SSE2 32bpp blitter factory. */
static FBlitter_32bppSSE2 iFBlitter_32bppSSE2;

/**
 * Resolve repeated animated sprites once per palette change without mutating encoded data.
 * @param sprite Encoded source sprite.
 * @param zoom Selected source mipmap.
 * @param height Mipmap height.
 * @return Cached colours and exact per-pixel dependencies, or null for a large sprite.
 */
const Blitter_32bppSSE2::CachedColourSprite *Blitter_32bppSSE2::GetScaledColourSprite(const SpriteData *sprite, ZoomLevel zoom, int height)
{
	constexpr size_t limit = 4 * 1024 * 1024;
	constexpr size_t entry_limit = 128 * 1024;
	const auto &info = sprite->infos[zoom];
	const size_t count = static_cast<size_t>(info.sprite_width) * height;
	const size_t bytes = count * (sizeof(Colour) + sizeof(uint32_t)) + sizeof(CachedColourSprite) + 64;
	if (bytes > entry_limit) return nullptr;
	const std::pair key{sprite->identity, zoom};
	auto it = this->scaled_colour_sprites.find(key);
	const bool fresh = it == this->scaled_colour_sprites.end();
	if (fresh) {
		while (this->scaled_colour_bytes + bytes > limit || this->scaled_colour_sprites.size() >= 256) {
			auto oldest = std::min_element(this->scaled_colour_sprites.begin(), this->scaled_colour_sprites.end(),
					[](const auto &a, const auto &b) { return a.second.used < b.second.used; });
			this->scaled_colour_bytes -= oldest->second.bytes;
			this->scaled_colour_sprites.erase(oldest);
		}
		it = this->scaled_colour_sprites.try_emplace(key).first;
		it->second.colours.resize(count);
		it->second.masks.resize(count);
		it->second.bytes = bytes;
		this->scaled_colour_bytes += bytes;
	}
	auto &cached = it->second;
	cached.used = ++this->scaled_colour_used;
	if (!fresh && std::equal(cached.palette.begin(), cached.palette.end(), _cur_palette.palette + PALETTE_ANIM_START)) return &cached;
	std::copy_n(_cur_palette.palette + PALETTE_ANIM_START, PALETTE_ANIM_SIZE, cached.palette.begin());
	const MapValue *mapping = reinterpret_cast<const MapValue *>(sprite->data + info.mv_offset);
	for (int y = 0; y < height; ++y) {
		const Colour *source = reinterpret_cast<const Colour *>(sprite->data + info.sprite_offset + static_cast<size_t>(y) * info.sprite_line_size) + META_LENGTH;
		for (int x = 0; x < info.sprite_width; ++x, ++mapping) {
			const size_t offset = static_cast<size_t>(y) * info.sprite_width + x;
			Colour colour = source[x];
			uint32_t mask = 0;
			if (colour.a != 0 && mapping->m >= PALETTE_ANIM_START && mapping->m < PALETTE_ANIM_START + PALETTE_ANIM_SIZE) {
				const uint8_t alpha = colour.a;
				colour = AdjustBrightness(this->LookupColourInPalette(mapping->m), mapping->v);
				colour.a = alpha;
				mask = uint32_t{1} << (mapping->m - PALETTE_ANIM_START);
			}
			cached.colours[offset] = colour;
			cached.masks[offset] = mask;
		}
	}
	return &cached;
}

/** SSE sprites already contain dense pixels, so fractional drawing needs no RLE cache. */
bool Blitter_32bppSSE2::DrawScaled(ScaledBlitterParams *bp, BlitterMode mode)
{
	ZoomLevel zoom = this->SelectScaledSpriteZoom(*bp);
	if (ScaleByZoom(1, zoom) > bp->step) return false;
	const Blitter_32bppSSE_Base::SpriteData *sprite = static_cast<const Blitter_32bppSSE_Base::SpriteData *>(bp->sprite);
	const auto &info = sprite->infos[zoom];
	const int width = info.sprite_width;
	const int height = UnScaleByZoom(bp->sprite_height, zoom);
	if (bp->nearest) {
		const bool static_colours = sprite->flags.Test(SpriteFlag::NoAnim) &&
				(mode == BlitterMode::Normal || (mode == BlitterMode::ColourRemap && sprite->flags.Test(SpriteFlag::NoRemap)));
		const CachedColourSprite *cached = !static_colours && mode == BlitterMode::Normal ? this->GetScaledColourSprite(sprite, zoom, height) : nullptr;
		if (!static_colours && mode == BlitterMode::Normal && cached == nullptr) {
			if (this->scaled_colour_row.size() < static_cast<size_t>(width)) this->scaled_colour_row.resize(width);
		} else if (!static_colours && mode != BlitterMode::Normal && this->scaled_rows.size() < static_cast<size_t>(width)) {
			this->scaled_rows.resize(width);
		}
		return this->DrawScaledNearestRows(bp, width, height, ScaleByZoom(1, zoom),
				[&](int y, Colour *destination, const int *columns, int first, int last) -> uint32_t {
					const Colour *source = reinterpret_cast<const Colour *>(sprite->data + info.sprite_offset + static_cast<size_t>(y) * info.sprite_line_size);
					/* Native SSE rows already describe their empty margins. Skipping
					 * those samples matters for ground diamonds and sparse trees. */
					const int source_first = source[0].data;
					const int source_last = width - source[1].data;
					if (source_first >= source_last) return 0;
					if (columns[first] < source_first) first = static_cast<int>(std::lower_bound(columns + first, columns + last, source_first) - columns);
					if (first >= last) return 0;
					if (columns[last - 1] >= source_last) last = static_cast<int>(std::lower_bound(columns + first, columns + last, source_last) - columns);
					if (first >= last) return 0;
					source += META_LENGTH;
					if (static_colours) {
						this->WriteScaledColourRow(destination, source, columns, first, last);
						return 0;
					}
					if (cached != nullptr) {
						const size_t offset = static_cast<size_t>(y) * width;
						uint32_t mask = 0;
						for (int x = first; x < last; ++x) mask |= cached->masks[offset + columns[x]];
						this->WriteScaledColourRow(destination, cached->colours.data() + offset, columns, first, last);
						return mask;
					}
					const MapValue *mapping = reinterpret_cast<const MapValue *>(sprite->data + info.mv_offset) + static_cast<size_t>(y) * width;
					if (mode == BlitterMode::Normal) {
						uint32_t mask = 0;
						Colour *row = this->scaled_colour_row.data();
						/* Only animated entries need a new palette lookup each tick. */
						for (int x = first; x < last; ++x) {
							const int column = columns[x];
							Colour colour = source[column];
							const MapValue value = mapping[column];
							if (colour.a != 0 && value.m >= PALETTE_ANIM_START && value.m < PALETTE_ANIM_START + PALETTE_ANIM_SIZE) {
								const uint8_t alpha = colour.a;
								colour = AdjustBrightness(this->LookupColourInPalette(value.m), value.v);
								colour.a = alpha;
								mask |= uint32_t{1} << (value.m - PALETTE_ANIM_START);
							}
							row[column] = colour;
						}
						this->WriteScaledColourRow(destination, row, columns, first, last);
						return mask;
					}
					Pixel *row = this->scaled_rows.data();
					for (int x = columns[first]; x <= columns[last - 1]; ++x) {
						row[x] = {source[x].r, source[x].g, source[x].b, source[x].a, mapping[x].m, mapping[x].v};
					}
					return this->DrawScaledNearestRow(destination, row, columns, first, last, mode, bp->remap);
				});
	}
	return this->DrawScaledRows(bp, mode, width, height, ScaleByZoom(1, zoom), [&](int y, Pixel *row) {
		const Colour *colour = reinterpret_cast<const Colour *>(sprite->data + info.sprite_offset +
				static_cast<size_t>(y) * info.sprite_line_size) + META_LENGTH;
		const MapValue *mapping = reinterpret_cast<const MapValue *>(sprite->data + info.mv_offset) + static_cast<size_t>(y) * width;
		for (int x = 0; x < width; x++) {
			row[x] = {colour[x].r, colour[x].g, colour[x].b, colour[x].a, mapping[x].m, mapping[x].v};
		}
	});
}

Sprite *Blitter_32bppSSE_Base::Encode(SpriteType sprite_type, const SpriteLoader::SpriteCollection &sprite, SpriteAllocator &allocator)
{
	/* First uint32_t of a line = the number of transparent pixels from the left.
	 * Second uint32_t of a line = the number of transparent pixels from the right.
	 * Then all RGBA then all MV.
	 */
	ZoomLevel zoom_min = ZoomLevel::Min;
	ZoomLevel zoom_max = ZoomLevel::Min;
	if (sprite_type != SpriteType::Font) {
		zoom_min = _settings_client.gui.zoom_min;
		zoom_max = _settings_client.gui.zoom_max;
		if (zoom_max == zoom_min) zoom_max = ZoomLevel::Max;
	}

	/* Calculate sizes and allocate. */
	SpriteData sd{};
	static std::atomic<uint64_t> next_identity{1};
	sd.identity = next_identity.fetch_add(1, std::memory_order_relaxed);
	uint all_sprites_size = 0;
	for (ZoomLevel z = zoom_min; z <= zoom_max; z++) {
		const SpriteLoader::Sprite *src_sprite = &sprite[z];
		auto &info = sd.infos[z];
		info.sprite_width = src_sprite->width;
		info.sprite_offset = all_sprites_size;
		info.sprite_line_size = sizeof(Colour) * src_sprite->width + sizeof(uint32_t) * META_LENGTH;

		const uint rgba_size = info.sprite_line_size * src_sprite->height;
		info.mv_offset = all_sprites_size + rgba_size;

		const uint mv_size = sizeof(MapValue) * src_sprite->width * src_sprite->height;
		all_sprites_size += rgba_size + mv_size;
	}

	Sprite *dst_sprite = allocator.Allocate<Sprite>(sizeof(Sprite) + sizeof(SpriteData) + all_sprites_size);
	const auto &root_sprite = sprite.Root();
	dst_sprite->height = root_sprite.height;
	dst_sprite->width = root_sprite.width;
	dst_sprite->x_offs = root_sprite.x_offs;
	dst_sprite->y_offs = root_sprite.y_offs;
	std::copy_n(reinterpret_cast<std::byte *>(&sd), sizeof(SpriteData), dst_sprite->data);

	/* Copy colours and determine flags. */
	bool has_remap = false;
	bool has_anim = false;
	bool has_translucency = false;
	for (ZoomLevel z = zoom_min; z <= zoom_max; z++) {
		const SpriteLoader::Sprite *src_sprite = &sprite[z];
		const SpriteLoader::CommonPixel *src = (const SpriteLoader::CommonPixel *) src_sprite->data;
		const auto &info = sd.infos[z];
		Colour *dst_rgba_line = reinterpret_cast<Colour *>(&dst_sprite->data[sizeof(SpriteData) + info.sprite_offset]);
		MapValue *dst_mv = reinterpret_cast<MapValue *>(&dst_sprite->data[sizeof(SpriteData) + info.mv_offset]);
		for (uint y = src_sprite->height; y != 0; y--) {
			Colour *dst_rgba = dst_rgba_line + META_LENGTH;
			for (uint x = src_sprite->width; x != 0; x--) {
				if (src->a != 0) {
					dst_rgba->a = src->a;
					if (src->a != 0 && src->a != 255) has_translucency = true;
					dst_mv->m = src->m;
					if (src->m != 0) {
						/* Do some accounting for flags. */
						has_remap = true;
						if (src->m >= PALETTE_ANIM_START) has_anim = true;

						/* Get brightest value (or default brightness if it's a black pixel). */
						const uint8_t rgb_max = std::max({src->r, src->g, src->b});
						dst_mv->v = (rgb_max == 0) ? DEFAULT_BRIGHTNESS : rgb_max;

						/* Pre-convert the mapping channel to a RGB value. */
						const Colour colour = AdjustBrightneSSE(Blitter_32bppBase::LookupColourInPalette(src->m), dst_mv->v);
						dst_rgba->r = colour.r;
						dst_rgba->g = colour.g;
						dst_rgba->b = colour.b;
					} else {
						dst_rgba->r = src->r;
						dst_rgba->g = src->g;
						dst_rgba->b = src->b;
						dst_mv->v = DEFAULT_BRIGHTNESS;
					}
				} else {
					dst_rgba->data = 0;
					*(uint16_t*) dst_mv = 0;
				}
				dst_rgba++;
				dst_mv++;
				src++;
			}

			/* Count the number of transparent pixels from the left. */
			dst_rgba = dst_rgba_line + META_LENGTH;
			uint32_t nb_pix_transp = 0;
			for (uint x = src_sprite->width; x != 0; x--) {
				if (dst_rgba->a == 0) nb_pix_transp++;
				else break;
				dst_rgba++;
			}
			dst_rgba_line->data = nb_pix_transp;

			Colour *nb_right = dst_rgba_line + 1;
			dst_rgba_line = reinterpret_cast<Colour *>(reinterpret_cast<std::byte *>(dst_rgba_line) + info.sprite_line_size);

			/* Count the number of transparent pixels from the right. */
			dst_rgba = dst_rgba_line - 1;
			nb_pix_transp = 0;
			for (uint x = src_sprite->width; x != 0; x--) {
				if (dst_rgba->a == 0) nb_pix_transp++;
				else break;
				dst_rgba--;
			}
			nb_right->data = nb_pix_transp;
		}
	}

	/* Store sprite flags. */
	sd.flags = {};
	if (has_translucency) sd.flags.Set(SpriteFlag::Translucent);
	if (!has_remap) sd.flags.Set(SpriteFlag::NoRemap);
	if (!has_anim) sd.flags.Set(SpriteFlag::NoAnim);
	std::copy_n(reinterpret_cast<std::byte *>(&sd), sizeof(SpriteData), dst_sprite->data);

	return dst_sprite;
}

#endif /* WITH_SSE */
