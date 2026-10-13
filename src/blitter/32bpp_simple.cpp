/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file 32bpp_simple.cpp Implementation of the simple 32 bpp blitter. */

#include "../stdafx.h"
#include "../core/math_func.hpp"
#include "../zoom_func.h"
#include "../palette_func.h"
#include "../settings_type.h"
#include "32bpp_simple.hpp"

#include "../table/sprites.h"

#include "../safeguards.h"

/** Instantiation of the simple 32bpp blitter factory. */
static FBlitter_32bppSimple iFBlitter_32bppSimple;

void Blitter_32bppSimple::DrawPixel(Colour *dst, const Pixel *src, BlitterMode mode, const uint8_t *remap)
{
	switch (mode) {
		case BlitterMode::ColourRemap:
			/* In case the m-channel is zero, do not remap this pixel in any way */
			if (src->m == 0) {
				if (src->a != 0) *dst = ComposeColourRGBA(src->r, src->g, src->b, src->a, *dst);
			} else {
				if (remap[src->m] != 0) *dst = ComposeColourPA(AdjustBrightness(this->LookupColourInPalette(remap[src->m]), src->v), src->a, *dst);
			}
			break;

		case BlitterMode::CrashRemap:
			if (src->m == 0) {
				if (src->a != 0) {
					uint8_t g = MakeDark(src->r, src->g, src->b);
					*dst = ComposeColourRGBA(g, g, g, src->a, *dst);
				}
			} else {
				if (remap[src->m] != 0) *dst = ComposeColourPA(AdjustBrightness(this->LookupColourInPalette(remap[src->m]), src->v), src->a, *dst);
			}
			break;

		case BlitterMode::BlackRemap:
			if (src->a != 0) {
				*dst = Colour(0, 0, 0);
			}
			break;

		case BlitterMode::Transparent:
			/* Make the current colour a bit more black, so it looks like this image is transparent */
			if (src->a != 0) {
				*dst = MakeTransparent(*dst, 192);
			}
			break;

		case BlitterMode::TransparentRemap:
			/* Apply custom transparency remap. */
			if (src->a != 0) {
				*dst = this->LookupColourInPalette(remap[GetNearestColourIndex(*dst)]);
			}
			break;

		default:
			if (src->a != 0) *dst = ComposeColourRGBA(src->r, src->g, src->b, src->a, *dst);
			break;
	}
}

void Blitter_32bppSimple::Draw(Blitter::BlitterParams *bp, BlitterMode mode, ZoomLevel zoom)
{
	const Blitter_32bppSimple::Pixel *src, *src_line;
	Colour *dst, *dst_line;

	/* Find where to start reading in the source sprite */
	src_line = (const Blitter_32bppSimple::Pixel *)bp->sprite + (bp->skip_top * bp->sprite_width + bp->skip_left) * ScaleByZoom(1, zoom);
	dst_line = (Colour *)bp->dst + bp->top * bp->pitch + bp->left;

	for (int y = 0; y < bp->height; y++) {
		dst = dst_line;
		dst_line += bp->pitch;

		src = src_line;
		src_line += bp->sprite_width * ScaleByZoom(1, zoom);

		for (int x = 0; x < bp->width; x++) {
			this->DrawPixel(dst, src, mode, bp->remap);
			dst++;
			src += ScaleByZoom(1, zoom);
		}
	}
}

/**
 * Choose a source mipmap without crossing a sub-sprite clip boundary.
 * A coarse source pixel straddling that boundary may contain excluded colours,
 * which cannot be removed by limiting its area-filter weight afterwards.
 * @param bp Fractional draw and source clipping parameters.
 * @return Coarsest usable source mipmap without crossing excluded edges.
 */
ZoomLevel Blitter_32bppSimple::SelectScaledSpriteZoom(const ScaledBlitterParams &bp)
{
	ZoomLevel zoom = _settings_client.gui.zoom_min;
	while (zoom < _settings_client.gui.zoom_max) {
		const int factor = ScaleByZoom(1, zoom + 1);
		if (factor > bp.step || bp.clip_left % factor != 0 || bp.clip_top % factor != 0 ||
				(bp.clip_right != 0 && (bp.sprite_width - bp.clip_right) % factor != 0) ||
				(bp.clip_bottom != 0 && (bp.sprite_height - bp.clip_bottom) % factor != 0)) break;
		zoom++;
	}
	return zoom;
}

/**
 * Resolve remapping before filtering, so a footprint can contain several company colours.
 * @param p Source pixel, updated with resolved palette colour and alpha.
 * @param mode Blitting mode.
 * @param remap Optional palette remapping table.
 */
void Blitter_32bppSimple::ResolveScaledPixel(Pixel &p, BlitterMode mode, const uint8_t *remap)
{
	if (p.a == 0) return;
	if (mode == BlitterMode::BlackRemap || mode == BlitterMode::Transparent || mode == BlitterMode::TransparentRemap) {
		/* Mask effects do not use the source RGB or source palette colour. */
		p.m = 0;
		return;
	}
	if (p.m != 0) {
		uint8_t index = p.m;
		if (mode == BlitterMode::ColourRemap || mode == BlitterMode::CrashRemap) index = remap[index];
		if (index == 0) {
			p.a = 0;
			return;
		}
		Colour colour = AdjustBrightness(this->LookupColourInPalette(index), p.v);
		p.m = index;
		p.r = colour.r;
		p.g = colour.g;
		p.b = colour.b;
	} else if (mode == BlitterMode::CrashRemap) {
		p.r = p.g = p.b = MakeDark(p.r, p.g, p.b);
	}
}

/**
 * Composite a resolved pixel; palette animation blitters override this to clear their metadata.
 * @param dst Destination pixel.
 * @param p Resolved source pixel.
 * @param mode Blitting mode.
 * @param remap Optional palette remapping table.
 */
void Blitter_32bppSimple::WriteScaledPixel(Colour *dst, const Pixel &p, BlitterMode mode, const uint8_t *remap)
{
	switch (mode) {
		case BlitterMode::BlackRemap:
			*dst = ComposeColourRGBA(0, 0, 0, p.a, *dst);
			break;
		case BlitterMode::Transparent:
			*dst = MakeTransparent(*dst, 1024 - p.a, 1024);
			break;
		case BlitterMode::TransparentRemap: {
			Colour colour = this->LookupColourInPalette(remap[GetNearestColourIndex(*dst)]);
			*dst = ComposeColourPA(colour, p.a, *dst);
			break;
		}
		default:
			*dst = ComposeColourRGBA(p.r, p.g, p.b, p.a, *dst);
			break;
	}
}

/**
 * Remove arithmetic noise near pixel and sub-sprite clipping boundaries.
 * Every clip boundary in a selected mipmap is a multiple of 1 / 32. Separate
 * dirty rectangles can reach the same coordinate through different additions.
 * Treating an almost exact boundary consistently prevents missing edge pixels.
 * @param value Source coordinate before arithmetic-noise correction.
 * @return Coordinate snapped only when extremely close to a clipping-grid boundary.
 */
static double SnapScaledSpriteCoordinate(double value)
{
	constexpr double grid = 1U << to_underlying(ZoomLevel::Max);
	/* Only values extremely close to a grid point can be snapped. Half-grid
	 * ties are outside that tolerance, so floor avoids the slower round call. */
	const double boundary = std::floor(value * grid + 0.5) / grid;
	return std::abs(value - boundary) <= 1e-9 ? boundary : value;
}

/** Whole sprites only need snapping at integer pixel edges. */
static int ScaledSpritePixel(double value)
{
	const double lower = std::floor(value);
	return static_cast<int>(lower) + (std::abs(value - (lower + 1)) <= 1e-9);
}

uint32_t Blitter_32bppSimple::DrawScaledNearestRow(Colour *destination, const Pixel *source, const int *columns,
		int first, int last, BlitterMode mode, const uint8_t *remap)
{
	const auto write = [&](int x, Colour colour, uint8_t alpha) {
		if (alpha == 255) {
			colour.a = 255;
			destination[x] = colour;
		} else {
			destination[x] = ComposeColourPA(colour, alpha, destination[x]);
		}
	};
	if (mode == BlitterMode::Normal) return this->DrawScaledColourRow<false>(source, columns, first, last, remap, write);
	if (mode == BlitterMode::ColourRemap) return this->DrawScaledColourRow<true>(source, columns, first, last, remap, write);
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

void Blitter_32bppSimple::WriteScaledColourRow(Colour *destination, const Colour *source, const int *columns, int first, int last)
{
	CompositeScaledColourRow(source, columns, first, last, [&](int x, Colour colour) {
		if (colour.a == 255) destination[x] = colour;
		else destination[x] = ComposeColourPA(colour, colour.a, destination[x]);
	});
}

/**
 * Share clipping and centre coordinates between decoded and already-coloured sprites.
 * @param bp Fractional source and destination parameters.
 * @param width Selected source mipmap width.
 * @param height Selected source mipmap height.
 * @param factor Virtual pixels per mipmap pixel.
 * @param context Opaque row-compositor context.
 * @param draw_row Callback compositing one source row at the selected columns.
 * @return True when the sprite was handled.
 */
bool Blitter_32bppSimple::DrawScaledNearestRows(ScaledBlitterParams *bp, int width, int height, double factor,
		const void *context, uint32_t (*draw_row)(const void *, int, Colour *, const int *, int, int))
{
	bp->palette_mask = 0;
	const double step = bp->step / factor;
	const double clip_left = bp->clip_left / factor;
	const double clip_top = bp->clip_top / factor;
	const double clip_right = bp->clip_right == 0 ? width : (bp->sprite_width - bp->clip_right) / factor;
	const double clip_bottom = bp->clip_bottom == 0 ? height : (bp->sprite_height - bp->clip_bottom) / factor;
	assert(step >= 1 && step <= 32);
	if (width <= 0 || height <= 0 || bp->width <= 0 || bp->height <= 0) return true;
	/* Centre sampling avoids constructing area-filter scanlines.
	 * Fractional viewports use it during motion and at rest to keep detail stable. */
	const int destination_width = bp->width;
	const int destination_height = bp->height;
	const int destination_pitch = bp->pitch;
	if (this->scaled_nearest_columns.size() < static_cast<size_t>(destination_width)) this->scaled_nearest_columns.resize(destination_width);
	int first_column = destination_width;
	int last_column = 0;
	const double origin_x = bp->src_x / factor;
	const double origin_y = bp->src_y / factor;
	const bool clipped_x = bp->clip_left != 0 || bp->clip_right != 0;
	const bool clipped_y = bp->clip_top != 0 || bp->clip_bottom != 0;
	for (int x = 0; x < destination_width; x++) {
		const double value = origin_x + (x + 0.5) * step;
		const double centre = clipped_x ? SnapScaledSpriteCoordinate(value) : value;
		const int cx = clipped_x ? static_cast<int>(std::floor(centre)) : ScaledSpritePixel(value);
		this->scaled_nearest_columns[x] = -1;
		if (cx < 0 || cx >= width || (clipped_x && (centre < clip_left || centre >= clip_right))) continue;
		this->scaled_nearest_columns[x] = cx;
		first_column = std::min(first_column, x);
		last_column = x + 1;
	}
	if (first_column >= last_column) return true;
	const int *columns = this->scaled_nearest_columns.data();
	Colour *destination = static_cast<Colour *>(bp->dst);
	uint32_t palette_mask = 0;
	for (int y = 0; y < destination_height; y++, destination += destination_pitch) {
		const double value = origin_y + (y + 0.5) * step;
		const double centre = clipped_y ? SnapScaledSpriteCoordinate(value) : value;
		const int cy = clipped_y ? static_cast<int>(std::floor(centre)) : ScaledSpritePixel(value);
		if (cy < 0 || cy >= height || (clipped_y && (centre < clip_top || centre >= clip_bottom))) continue;
		palette_mask |= draw_row(context, cy, destination, columns, first_column, last_column);
	}
	bp->palette_mask = palette_mask;
	return true;
}

/**
 * Separable area filter with cached horizontal scanlines. Optimized blitters
 * choose a mipmap whose output footprint spans at most three rows/columns.
 * Transparent edges use centre coverage so adjoining ground sprites do not
 * expose the destination through independently blended edges.
 * @param bp Fractional source, destination and clipping parameters.
 * @param mode Blitting mode.
 * @param width Selected source mipmap width in pixels.
 * @param height Selected source mipmap height in pixels.
 * @param factor Virtual pixels per selected mipmap pixel.
 * @param context Opaque row-decoder context.
 * @param read_row Callback decoding a source scanline into Pixel values.
 * @return True when fractional drawing is supported.
 */
bool Blitter_32bppSimple::DrawScaledRows(ScaledBlitterParams *bp, BlitterMode mode, int width, int height, double factor,
		const void *context, void (*read_row)(const void *, int, Pixel *))
{
	static_assert(PALETTE_ANIM_SIZE <= 32);
	bp->palette_mask = 0;
	const auto animated_mask = [](const Pixel &pixel, double weight) -> uint32_t {
		if (pixel.a == 0 || weight <= 0 || pixel.m < PALETTE_ANIM_START || pixel.m >= PALETTE_ANIM_START + PALETTE_ANIM_SIZE) return 0;
		return uint32_t{1} << (pixel.m - PALETTE_ANIM_START);
	};
	const double step = bp->step / factor;
	const double clip_left = bp->clip_left / factor;
	const double clip_top = bp->clip_top / factor;
	const double clip_right = bp->clip_right == 0 ? width : (bp->sprite_width - bp->clip_right) / factor;
	const double clip_bottom = bp->clip_bottom == 0 ? height : (bp->sprite_height - bp->clip_bottom) / factor;
	assert(step >= 1 && step <= 32);
	if (width <= 0 || height <= 0 || bp->width <= 0 || bp->height <= 0) return true;
	if (bp->nearest) {
		if (this->scaled_rows.size() < static_cast<size_t>(width)) this->scaled_rows.resize(width);
		Pixel *source_row = this->scaled_rows.data();
		int cached_row = -1;
		return this->DrawScaledNearestRows(bp, width, height, factor,
				[&](int y, Colour *destination, const int *columns, int first, int last) {
					if (y != cached_row) { read_row(context, y, source_row); cached_row = y; }
					return this->DrawScaledNearestRow(destination, source_row, columns, first, last, mode, bp->remap);
				});
	}
	const int row_count = static_cast<int>(std::ceil(step)) + 1;
	const int destination_width = bp->width;
	const int destination_height = bp->height;
	const int destination_pitch = bp->pitch;
	const uint8_t *remap = bp->remap;
	const bool remap_alpha = mode == BlitterMode::ColourRemap || mode == BlitterMode::CrashRemap;
	const size_t source_size = static_cast<size_t>(width) * row_count;
	const size_t filtered_size = static_cast<size_t>(bp->width) * row_count;
	if (this->scaled_rows.size() < source_size) this->scaled_rows.resize(source_size);
	if (this->scaled_columns.size() < static_cast<size_t>(bp->width)) this->scaled_columns.resize(bp->width);
	if (this->scaled_weights.size() < filtered_size) this->scaled_weights.resize(filtered_size);
	if (this->scaled_filtered_rows.size() < filtered_size) this->scaled_filtered_rows.resize(filtered_size);

	/* Source x coordinates and overlap weights are identical for every row. */
	int source_left = width;
	int source_right = 0;
	for (int x = 0; x < bp->width; x++) {
		auto &column = this->scaled_columns[x];
		column = {0, 0, 0, 0, true};
		const double sx0 = SnapScaledSpriteCoordinate(bp->src_x / factor + x * step);
		const double sx1 = SnapScaledSpriteCoordinate(sx0 + step);
		const double centre_x = SnapScaledSpriteCoordinate((sx0 + sx1) * 0.5);
		const int cx = static_cast<int>(std::floor(centre_x));
		if (cx < 0 || cx >= width || centre_x < clip_left || centre_x >= clip_right) continue;
		column.first = std::max(0, static_cast<int>(std::floor(sx0)));
		column.last = std::min(width, static_cast<int>(std::ceil(sx1)));
		column.centre = cx;
		source_left = std::min(source_left, column.first);
		source_right = std::max(source_right, column.last);
		column.edge = sx0 < clip_left || sx1 > clip_right;
		double *weights = this->scaled_weights.data() + static_cast<size_t>(x) * row_count;
		for (int ix = column.first; ix < column.last; ix++) {
			const double weight = std::max(0.0, std::min({static_cast<double>(ix + 1), sx1, clip_right}) -
					std::max({static_cast<double>(ix), sx0, clip_left}));
			weights[ix - column.first] = weight;
			column.coverage += weight;
		}
	}
	if (source_left >= source_right) return true;

	std::array<int, 33> cached;
	cached.fill(-1);
	const double inverse_area = 1 / (step * step);
	const double inverse_opaque_alpha = inverse_area / 255;
	const ScaledColumn *columns = this->scaled_columns.data();
	const Pixel *source_rows = this->scaled_rows.data();
	ScaledFilteredPixel *filtered_rows = this->scaled_filtered_rows.data();
	Colour *dst_line = static_cast<Colour *>(bp->dst);
	for (int y = 0; y < destination_height; y++, dst_line += destination_pitch) {
		const double sy0 = SnapScaledSpriteCoordinate(bp->src_y / factor + y * step);
		const double sy1 = SnapScaledSpriteCoordinate(sy0 + step);
		const double centre_y = SnapScaledSpriteCoordinate((sy0 + sy1) * 0.5);
		const int cy = static_cast<int>(std::floor(centre_y));
		if (cy < 0 || cy >= height || centre_y < clip_top || centre_y >= clip_bottom) continue;
		const int iy0 = std::max(0, static_cast<int>(std::floor(sy0)));
		const int iy1 = std::min(height, static_cast<int>(std::ceil(sy1)));
		const bool vertical_edge = sy0 < clip_top || sy1 > clip_bottom;
		std::array<double, 33> row_weights;
		std::array<const ScaledFilteredPixel *, 33> filtered_row_ptrs;
		std::array<uint32_t, 33> palette_weight_masks;
		for (int iy = iy0; iy < iy1; iy++) {
			row_weights[iy - iy0] = std::max(0.0, std::min({static_cast<double>(iy + 1), sy1, clip_bottom}) -
					std::max({static_cast<double>(iy), sy0, clip_top}));
			const int slot = iy % row_count;
			filtered_row_ptrs[iy - iy0] = filtered_rows + static_cast<size_t>(slot) * destination_width;
			palette_weight_masks[iy - iy0] = row_weights[iy - iy0] > 0 ? std::numeric_limits<uint32_t>::max() : 0;
			if (cached[slot] == iy) continue;
			Pixel *row = this->scaled_rows.data() + static_cast<size_t>(slot) * width;
			read_row(context, iy, row);
			bool row_opaque = true;
			/* Clipped repaint fragments only need colours from sampled columns. */
			for (int ix = source_left; ix < source_right; ix++) {
				this->ResolveScaledPixel(row[ix], mode, bp->remap);
				row_opaque &= row[ix].a == 255;
			}
			/* Keep the same arithmetic path for a source row regardless of dirty
			 * partitioning. Unsampled columns need only their effective alpha. */
			const auto is_opaque = [&](const Pixel &pixel) {
				return pixel.a == 255 && (!remap_alpha || pixel.m == 0 || remap[pixel.m] != 0);
			};
			for (int ix = 0; row_opaque && ix < source_left; ix++) row_opaque = is_opaque(row[ix]);
			for (int ix = source_right; row_opaque && ix < width; ix++) row_opaque = is_opaque(row[ix]);
			auto *filtered = this->scaled_filtered_rows.data() + static_cast<size_t>(slot) * bp->width;
			for (int x = 0; x < bp->width; x++) {
				const auto &column = this->scaled_columns[x];
				auto &out = filtered[x];
				out = {0, 0, 0, 0, 0, false, true};
				if (column.first == column.last) continue;
				const double *weights = this->scaled_weights.data() + static_cast<size_t>(x) * row_count;
				if (row_opaque) {
					/* Opaque water and terrain need no per-tap alpha multiplication. */
					for (int ix = column.first; ix < column.last; ix++) {
						const Pixel &pixel = row[ix];
						const double weight = weights[ix - column.first];
						out.palette_mask |= animated_mask(pixel, weight);
						out.r += weight * pixel.r;
						out.g += weight * pixel.g;
						out.b += weight * pixel.b;
					}
					out.r *= 255;
					out.g *= 255;
					out.b *= 255;
					out.a = column.coverage * 255;
				} else {
					for (int ix = column.first; ix < column.last; ix++) {
						const Pixel &pixel = row[ix];
						if (pixel.a == 0) {
							out.transparent = true;
							out.opaque = false;
							continue;
						}
						out.opaque &= pixel.a == 255;
						const double coverage = weights[ix - column.first];
						out.palette_mask |= animated_mask(pixel, coverage);
						const double weight = coverage * pixel.a;
						out.a += weight;
						out.r += weight * pixel.r;
						out.g += weight * pixel.g;
						out.b += weight * pixel.b;
					}
				}
			}
			cached[slot] = iy;
		}
		const Pixel *centre_row = source_rows + static_cast<size_t>(cy % row_count) * width;
		const int tap_count = iy1 - iy0;
		/* The normal mipmap path has two or three vertical taps. Select its
		 * specialization once per output row, keeping modulo and loop control
		 * outside the pixel loop. Wider reference filters retain the generic path. */
		const auto draw_row = [&]<int TapCount>() {
			for (int x = 0; x < destination_width; x++) {
				const auto &column = columns[x];
				if (column.first == column.last) continue;
				const Pixel &centre = centre_row[column.centre];
				if (centre.a == 0) continue;
				double alpha = 0, red = 0, green = 0, blue = 0;
				bool transparent_edge = column.edge || vertical_edge;
				bool opaque = !transparent_edge;
				uint32_t palette_mask = 0;
				const auto add_tap = [&](int index) {
					const auto &pixel = filtered_row_ptrs[index][x];
					const double weight = row_weights[index];
					palette_mask |= pixel.palette_mask & palette_weight_masks[index];
					alpha += weight * pixel.a;
					red += weight * pixel.r;
					green += weight * pixel.g;
					blue += weight * pixel.b;
					transparent_edge |= pixel.transparent;
					opaque &= pixel.opaque;
				};
				if constexpr (TapCount != 0) {
					add_tap(0);
					if constexpr (TapCount >= 2) add_tap(1);
					if constexpr (TapCount >= 3) add_tap(2);
				} else {
					for (int index = 0; index < tap_count; index++) add_tap(index);
				}
				if (alpha <= 0) continue;
				const double inverse_alpha = opaque ? inverse_opaque_alpha : 1 / alpha;
				Pixel out = centre;
				out.r = static_cast<uint8_t>(red * inverse_alpha + 0.5 + 1e-9);
				out.g = static_cast<uint8_t>(green * inverse_alpha + 0.5 + 1e-9);
				out.b = static_cast<uint8_t>(blue * inverse_alpha + 0.5 + 1e-9);
				out.a = opaque ? 255 : transparent_edge ? centre.a :
						static_cast<uint8_t>(std::min(255.0, alpha * inverse_area + 0.5 + 1e-9));
				if (out.a == 0) continue;
				this->WriteScaledPixel(dst_line + x, out, mode, remap);
				/* Destination remapping can depend on any animated background colour. */
				bp->palette_mask |= mode == BlitterMode::TransparentRemap ?
						std::numeric_limits<uint32_t>::max() >> (32 - PALETTE_ANIM_SIZE) : palette_mask;
			}
		};
		switch (tap_count) {
			case 1: draw_row.template operator()<1>(); break;
			case 2: draw_row.template operator()<2>(); break;
			case 3: draw_row.template operator()<3>(); break;
			default: draw_row.template operator()<0>(); break;
		}
	}
	return true;
}
/**
 * Draw the uncompressed reference sprite through the shared area sampler.
 * @param bp Fractional draw parameters.
 * @param mode Blitting mode.
 * @return True when the sprite was handled.
 */
bool Blitter_32bppSimple::DrawScaled(Blitter::ScaledBlitterParams *bp, BlitterMode mode)
{
	const Pixel *sprite = static_cast<const Pixel *>(bp->sprite);
	return this->DrawScaledRows(bp, mode, bp->sprite_width, bp->sprite_height, 1, [&](int y, Pixel *row) {
		std::copy_n(sprite + static_cast<size_t>(y) * bp->sprite_width, bp->sprite_width, row);
	});
}

void Blitter_32bppSimple::DrawColourMappingRect(void *dst, int width, int height, PaletteID pal)
{
	Colour *udst = (Colour *)dst;

	if (pal == PALETTE_TO_TRANSPARENT) {
		do {
			for (int i = 0; i != width; i++) {
				*udst = MakeTransparent(*udst, 154);
				udst++;
			}
			udst = udst - width + _screen.pitch;
		} while (--height);
		return;
	}
	if (pal == PALETTE_NEWSPAPER) {
		do {
			for (int i = 0; i != width; i++) {
				*udst = MakeGrey(*udst);
				udst++;
			}
			udst = udst - width + _screen.pitch;
		} while (--height);
		return;
	}

	Debug(Facility::Misc, Severity::Critical, "32bpp blitter doesn't know how to draw this colour table ('{}')", pal);
}

Sprite *Blitter_32bppSimple::Encode(SpriteType, const SpriteLoader::SpriteCollection &sprite, SpriteAllocator &allocator)
{
	const auto &root_sprite = sprite.Root();
	Blitter_32bppSimple::Pixel *dst;
	Sprite *dest_sprite = allocator.Allocate<Sprite>(sizeof(*dest_sprite) + static_cast<size_t>(root_sprite.height) * static_cast<size_t>(root_sprite.width) * sizeof(*dst));

	dest_sprite->height = root_sprite.height;
	dest_sprite->width = root_sprite.width;
	dest_sprite->x_offs = root_sprite.x_offs;
	dest_sprite->y_offs = root_sprite.y_offs;

	dst = reinterpret_cast<Blitter_32bppSimple::Pixel *>(dest_sprite->data);
	SpriteLoader::CommonPixel *src = reinterpret_cast<SpriteLoader::CommonPixel *>(root_sprite.data);

	for (int i = 0; i < root_sprite.height * root_sprite.width; i++) {
		if (src->m == 0) {
			dst[i].r = src->r;
			dst[i].g = src->g;
			dst[i].b = src->b;
			dst[i].a = src->a;
			dst[i].m = 0;
			dst[i].v = 0;
		} else {
			/* Get brightest value */
			uint8_t rgb_max = std::max({src->r, src->g, src->b});

			/* Black pixel (8bpp or old 32bpp image), so use default value */
			if (rgb_max == 0) rgb_max = DEFAULT_BRIGHTNESS;
			dst[i].v = rgb_max;

			/* Pre-convert the mapping channel to a RGB value */
			Colour colour = AdjustBrightness(this->LookupColourInPalette(src->m), dst[i].v);
			dst[i].r = colour.r;
			dst[i].g = colour.g;
			dst[i].b = colour.b;
			dst[i].a = src->a;
			dst[i].m = src->m;
		}
		src++;
	}

	return dest_sprite;
}
