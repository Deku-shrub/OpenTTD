/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file 32bpp_sse2.hpp SSE2 32 bpp blitter. */

#ifndef BLITTER_32BPP_SSE2_HPP
#define BLITTER_32BPP_SSE2_HPP

#ifdef WITH_SSE

#ifndef SSE_VERSION
#define SSE_VERSION 2
#endif

#ifndef SSE_TARGET
#define SSE_TARGET "sse2"
#endif

#ifndef FULL_ANIMATION
#define FULL_ANIMATION 0
#endif

#include "../cpu.h"
#include "32bpp_sse_type.h"
#include <unordered_map>

/** Base methods for 32bpp SSE blitters. */
class Blitter_32bppSSE_Base {
public:
	/** Ensure the destructor of the sub classes are called as well. */
	virtual ~Blitter_32bppSSE_Base() = default;

	struct MapValue {
		uint8_t m;
		uint8_t v;
	};
	static_assert(sizeof(MapValue) == 2);

	/** Helper for creating specialised functions for specific optimisations. */
	enum class ReadMode : uint8_t {
		WithSkip, ///< Use normal code for skipping empty pixels.
		WithMargin, ///< Use cached number of empty pixels at begin and end of line to reduce work.
		None, ///< No specialisation.
	};

	/** Helper for creating specialised functions for the case where the sprite width is odd or even. */
	enum class BlockType : uint8_t {
		Even, ///< An even number of pixels in the width; no need for a special case for the last pixel.
		Odd, ///< An odd number of pixels in the width; special case for the last pixel.
		None, ///< No specialisation for either case.
	};

	/** Helper for using specialised functions designed to prevent whenever it's possible things like:
	 *  - IO (reading video buffer),
	 *  - calculations (alpha blending),
	 *  - heavy branching (remap lookups and animation buffer handling).
	 */
	enum class SpriteFlag : uint8_t {
		Translucent, ///< The sprite has at least 1 translucent pixel.
		NoRemap, ///< The sprite has no remappable colour pixel.
		NoAnim, ///< The sprite has no palette animated pixel.
	};

	/** Bitset of \c SpriteFlag elements. */
	using SpriteFlags = EnumBitSet<SpriteFlag, uint8_t>;

	/** Data stored about a (single) sprite. */
	struct SpriteInfo {
		uint32_t sprite_offset = 0;    ///< The offset to the sprite data.
		uint32_t mv_offset = 0;        ///< The offset to the map value data.
		uint16_t sprite_line_size = 0; ///< The size of a single line (pitch).
		uint16_t sprite_width = 0;     ///< The width of the sprite.
	};
	struct SpriteData {
		SpriteFlags flags{};
		uint64_t identity = 0; ///< Changes on re-encoding even when the sprite cache reuses its address.
		SpriteCollMap<SpriteInfo> infos{};
		uint8_t data[]; ///< Data, all zoomlevels.
	};

	Sprite *Encode(SpriteType sprite_type, const SpriteLoader::SpriteCollection &sprite, SpriteAllocator &allocator);
};

/** The SSE2 32 bpp blitter (without palette animation). */
class Blitter_32bppSSE2 : public Blitter_32bppSimple, public Blitter_32bppSSE_Base {
private:
	std::vector<Colour> scaled_colour_row; ///< Reusable scanline with current animated palette colours.
	/** Resolved colours for one small animated mipmap. */
	struct CachedColourSprite {
		std::vector<Colour> colours; ///< Dense RGBA pixels without native row metadata.
		std::vector<uint32_t> masks; ///< Animated palette entry contributing to each pixel.
		std::array<Colour, PALETTE_ANIM_SIZE> palette; ///< Palette values used to resolve the colours.
		uint64_t used = 0; ///< Last use for bounded eviction.
		size_t bytes = 0; ///< Charged storage including entry overhead.
	};
	/** Hash an encoding identity and mipmap level without retaining sprite pointers. */
	struct ColourSpriteHash {
		/**
		 * @param key Encoding identity and selected mipmap.
		 * @return Hash for the resolved-colour cache.
		 */
		size_t operator()(const std::pair<uint64_t, ZoomLevel> &key) const { return std::hash<uint64_t>{}((key.first << 3) ^ to_underlying(key.second)); }
	};
	std::unordered_map<std::pair<uint64_t, ZoomLevel>, CachedColourSprite, ColourSpriteHash> scaled_colour_sprites; ///< Bounded cache for repeated animated sprites.
	size_t scaled_colour_bytes = 0; ///< Charged cache storage.
	uint64_t scaled_colour_used = 0; ///< Monotonic cache-use counter.
	const CachedColourSprite *GetScaledColourSprite(const SpriteData *sprite, ZoomLevel zoom, int height);

public:
	void Draw(Blitter::BlitterParams *bp, BlitterMode mode, ZoomLevel zoom) override;
	bool DrawScaled(ScaledBlitterParams *bp, BlitterMode mode) override;
	template <BlitterMode mode, Blitter_32bppSSE_Base::ReadMode read_mode, Blitter_32bppSSE_Base::BlockType bt_last, bool translucent>
	void Draw(const Blitter::BlitterParams *bp, ZoomLevel zoom);

	Sprite *Encode(SpriteType sprite_type, const SpriteLoader::SpriteCollection &sprite, SpriteAllocator &allocator) override
	{
		return Blitter_32bppSSE_Base::Encode(sprite_type, sprite, allocator);
	}

	std::string_view GetName() override { return "32bpp-sse2"; }
};

/** Factory for the SSE2 32 bpp blitter (without palette animation). */
class FBlitter_32bppSSE2 : public BlitterFactory {
public:
	FBlitter_32bppSSE2() : BlitterFactory("32bpp-sse2", "32bpp SSE2 Blitter (no palette animation)", HasCPUIDFlag(1, 3, 26)) {}
	std::unique_ptr<Blitter> CreateInstance() override { return std::make_unique<Blitter_32bppSSE2>(); }
};

#endif /* WITH_SSE */
#endif /* BLITTER_32BPP_SSE2_HPP */
