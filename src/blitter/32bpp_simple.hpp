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
	/** Horizontal sampling footprint, shared by all output rows of one sprite. */
	struct ScaledColumn {
		int first; ///< First source column in the sampling footprint.
		int last; ///< Last source column in the sampling footprint.
		int centre; ///< Source column containing the output pixel centre.
		double coverage; ///< Source-space footprint width.
		bool edge; ///< Whether the footprint touches a transparent or clipped edge.
	};
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
	 * @tparam ReadRow Source scanline decoder type.
	 * @param bp Fractional source, destination and clipping parameters.
	 * @param mode Blitting mode.
	 * @param width Width of the selected source mipmap in pixels.
	 * @param height Height of the selected source mipmap in pixels.
	 * @param factor Virtual pixels per selected mipmap pixel.
	 * @param read_row Callable decoding one source scanline.
	 * @return True when fractional drawing is supported.
	 */
	template <typename ReadRow>
	bool DrawScaledRows(ScaledBlitterParams *bp, BlitterMode mode, int width, int height, double factor, const ReadRow &read_row)
	{
		return this->DrawScaledRows(bp, mode, width, height, factor, &read_row, [](const void *context, int y, Pixel *row) {
			(*static_cast<const ReadRow *>(context))(y, row);
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
