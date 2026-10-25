/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file 32bpp_simple.hpp Simple 32 bpp blitter. */

#ifndef BLITTER_32BPP_SIMPLE_HPP
#define BLITTER_32BPP_SIMPLE_HPP

#include "32bpp_base.hpp"
#include "factory.hpp"

/** The most trivial 32 bpp blitter (without palette animation). */
class Blitter_32bppSimple : public Blitter_32bppBase {
protected:
	struct Pixel {
		uint8_t r;  ///< Red-channel
		uint8_t g;  ///< Green-channel
		uint8_t b;  ///< Blue-channel
		uint8_t a;  ///< Alpha-channel
		uint8_t m;  ///< Remap-channel
		uint8_t v;  ///< Brightness-channel
	};
	/**
	 * Composite one traditional sprite pixel.
	 * @param dst Destination pixel.
	 * @param src Source sprite pixel.
	 * @param mode Blitting mode.
	 * @param remap Optional palette remapping table.
	 */
	void DrawPixel(Colour *dst, const Pixel *src, BlitterMode mode, const uint8_t *remap);
	/** Scratch scanlines; optimized blitters need at most three rows of the selected mipmap. */
	std::vector<Pixel> scaled_rows;
	std::vector<int> scaled_nearest_columns; ///< Compact source indices for centre sampling.
	/** Horizontal sampling footprint, shared by all output rows of one sprite. */
	struct ScaledColumn {
		int first; ///< First source column in the sampling footprint.
		int last; ///< Last source column in the sampling footprint.
		int centre; ///< Source column containing the output pixel centre.
		double coverage; ///< Source-space footprint width.
		bool edge; ///< Whether the footprint touches a transparent or clipped edge.
	};
	/**
	 * Resolve sampled colours with format-specific output selected once per row.
	 * @tparam TRemap Whether to apply the source palette remapping table.
	 * @tparam TWritePixel Callable writing a resolved colour and alpha at an output column.
	 * @param source Decoded source scanline.
	 * @param columns Source columns selected for the output scanline.
	 * @param first First output column to draw.
	 * @param last Exclusive last output column to draw.
	 * @param remap Optional palette remapping table.
	 * @param write_pixel Output-format callback.
	 * @return Animated palette entries used by the scanline.
	 */
	template <bool TRemap, typename TWritePixel>
	uint32_t DrawScaledColourRow(const Pixel *source, const int *columns, int first, int last,
			const uint8_t *remap, const TWritePixel &write_pixel)
	{
		uint32_t mask = 0;
		for (int x = first; x < last; ++x) {
			if (columns[x] < 0) continue;
			const Pixel &pixel = source[columns[x]];
			if (pixel.a == 0) continue;
			uint8_t index = pixel.m;
			Colour colour(pixel.r, pixel.g, pixel.b);
			if (index != 0) {
				if constexpr (TRemap) index = remap[index];
				if (index == 0) continue;
				colour = AdjustBrightness(this->LookupColourInPalette(index), pixel.v);
				if (index >= PALETTE_ANIM_START && index < PALETTE_ANIM_START + PALETTE_ANIM_SIZE) mask |= uint32_t{1} << (index - PALETTE_ANIM_START);
			}
			write_pixel(x, colour, pixel.a);
		}
		return mask;
	}
	/**
	 * Composite a centre-sampled scanline without per-pixel output-format dispatch.
	 * @param destination Destination scanline.
	 * @param source Decoded source scanline.
	 * @param columns Selected source columns.
	 * @param first First output column to draw.
	 * @param last Exclusive last output column to draw.
	 * @param mode Blitting mode.
	 * @param remap Optional palette remapping table.
	 * @return Animated palette entries used by the scanline.
	 */
	virtual uint32_t DrawScaledNearestRow(Colour *destination, const Pixel *source, const int *columns,
			int first, int last, BlitterMode mode, const uint8_t *remap);
	/**
	 * Sample native colours without decoding or resolving their static palettes again.
	 * @tparam TWritePixel Output-format callback type.
	 * @param source Already-coloured source scanline.
	 * @param columns Selected source columns.
	 * @param first First output column.
	 * @param last Exclusive last output column.
	 * @param write_pixel Callback writing a sampled colour.
	 */
	template <typename TWritePixel>
	static void CompositeScaledColourRow(const Colour *source, const int *columns, int first, int last, const TWritePixel &write_pixel)
	{
		for (int x = first; x < last; ++x) {
			if (columns[x] < 0) continue;
			const Colour colour = source[columns[x]];
			if (colour.a != 0) write_pixel(x, colour);
		}
	}
	/**
	 * Composite precoloured samples and update destination-format metadata.
	 * @param destination Destination scanline.
	 * @param source Source scanline with palette colours already resolved.
	 * @param columns Selected source columns.
	 * @param first First output column.
	 * @param last Exclusive last output column.
	 */
	virtual void WriteScaledColourRow(Colour *destination, const Colour *source, const int *columns, int first, int last);
	bool DrawScaledNearestRows(ScaledBlitterParams *bp, int width, int height, double factor,
			const void *context, uint32_t (*draw_row)(const void *, int, Colour *, const int *, int, int));
	/**
	 * Adapt a row compositor without allocating a callable for each sprite.
	 * @tparam TDrawRow Row-compositor type.
	 * @param bp Fractional source and destination parameters.
	 * @param width Source mipmap width.
	 * @param height Source mipmap height.
	 * @param factor Virtual pixels per mipmap pixel.
	 * @param draw_row Row-compositor callback.
	 * @return True when the sprite was handled.
	 */
	template <typename TDrawRow>
	bool DrawScaledNearestRows(ScaledBlitterParams *bp, int width, int height, double factor, const TDrawRow &draw_row)
	{
		return this->DrawScaledNearestRows(bp, width, height, factor, &draw_row,
				[](const void *context, int y, Colour *destination, const int *columns, int first, int last) {
					return (*static_cast<const TDrawRow *>(context))(y, destination, columns, first, last);
				});
	}
	/** Premultiplied, horizontally filtered source pixel. */
	struct ScaledFilteredPixel {
		double r; ///< Premultiplied red contribution.
		double g; ///< Premultiplied green contribution.
		double b; ///< Premultiplied blue contribution.
		double a; ///< Accumulated alpha coverage.
		uint32_t palette_mask; ///< Animated-palette entries contributing to this footprint.
		bool transparent; ///< Whether the whole footprint is transparent.
		bool opaque; ///< Whether the whole footprint is opaque.
	};
	std::vector<ScaledColumn> scaled_columns; ///< Reusable horizontal sampling footprints.
	std::vector<double> scaled_weights; ///< Reusable source-column filter weights.
	std::vector<ScaledFilteredPixel> scaled_filtered_rows; ///< Reusable horizontally filtered source scanlines.
	void ResolveScaledPixel(Pixel &pixel, BlitterMode mode, const uint8_t *remap);
	static ZoomLevel SelectScaledSpriteZoom(const ScaledBlitterParams &bp);
	virtual void WriteScaledPixel(Colour *dst, const Pixel &pixel, BlitterMode mode, const uint8_t *remap);
	bool DrawScaledRows(ScaledBlitterParams *bp, BlitterMode mode, int width, int height, double factor,
			const void *context, void (*read_row)(const void *, int, Pixel *));
	/**
	 * Adapt row readers without allocating a type-erased function object per sprite.
	 * @tparam TReadRow Source scanline decoder type.
	 * @param bp Fractional source, destination and clipping parameters.
	 * @param mode Blitting mode.
	 * @param width Width of the selected source mipmap in pixels.
	 * @param height Height of the selected source mipmap in pixels.
	 * @param factor Virtual pixels per selected mipmap pixel.
	 * @param read_row Callable decoding one source scanline.
	 * @return True when fractional drawing is supported.
	 */
	template <typename TReadRow>
	bool DrawScaledRows(ScaledBlitterParams *bp, BlitterMode mode, int width, int height, double factor, const TReadRow &read_row)
	{
		return this->DrawScaledRows(bp, mode, width, height, factor, &read_row, [](const void *context, int y, Pixel *row) {
			(*static_cast<const TReadRow *>(context))(y, row);
		});
	}
public:
	void Draw(Blitter::BlitterParams *bp, BlitterMode mode, ZoomLevel zoom) override;
	bool DrawScaled(Blitter::ScaledBlitterParams *bp, BlitterMode mode) override;
	bool SupportsSmoothZoom() const override { return true; }
	void DrawColourMappingRect(void *dst, int width, int height, PaletteID pal) override;
	Sprite *Encode(SpriteType sprite_type, const SpriteLoader::SpriteCollection &sprite, SpriteAllocator &allocator) override;

	std::string_view GetName() override { return "32bpp-simple"; }
};

/** Factory for the simple 32 bpp blitter. */
class FBlitter_32bppSimple : public BlitterFactory {
public:
	FBlitter_32bppSimple() : BlitterFactory("32bpp-simple", "32bpp Simple Blitter (no palette animation)") {}
	std::unique_ptr<Blitter> CreateInstance() override { return std::make_unique<Blitter_32bppSimple>(); }
};

#endif /* BLITTER_32BPP_SIMPLE_HPP */
