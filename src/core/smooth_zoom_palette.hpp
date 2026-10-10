/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file smooth_zoom_palette.hpp Palette dependencies of fractionally drawn screen blocks. */

#ifndef SMOOTH_ZOOM_PALETTE_HPP
#define SMOOTH_ZOOM_PALETTE_HPP

#include <algorithm>
#include <cstdint>
#include <vector>

/** Conservative palette dependencies. Partial repaints retain dependencies outside their clip. */
class SmoothZoomPaletteTracker {
	int width = 0;
	int height = 0;
	int columns = 0;
	int rows = 0;
	std::vector<uint32_t> dependencies;
	std::vector<uint32_t> scroll_source;
	bool have_dependencies = false;

public:
	static constexpr int BLOCK_WIDTH = 64;
	static constexpr int BLOCK_HEIGHT = 8;

	void Resize(int width, int height)
	{
		this->width = std::max(0, width);
		this->height = std::max(0, height);
		this->columns = (this->width + BLOCK_WIDTH - 1) / BLOCK_WIDTH;
		this->rows = (this->height + BLOCK_HEIGHT - 1) / BLOCK_HEIGHT;
		this->dependencies.assign(static_cast<size_t>(this->columns) * this->rows, 0);
		this->have_dependencies = false;
	}

	/** Register a sprite's palette dependencies; right and bottom are exclusive. */
	void Register(int left, int top, int right, int bottom, uint32_t mask)
	{
		left = std::max(0, left);
		top = std::max(0, top);
		right = std::min(this->width, right);
		bottom = std::min(this->height, bottom);
		if (mask == 0 || left >= right || top >= bottom) return;
		this->have_dependencies = true;
		for (int x = left / BLOCK_WIDTH; x < (right + BLOCK_WIDTH - 1) / BLOCK_WIDTH; x++) {
			for (int y = top / BLOCK_HEIGHT; y < (bottom + BLOCK_HEIGHT - 1) / BLOCK_HEIGHT; y++) {
				this->dependencies[static_cast<size_t>(x) * this->rows + y] |= mask;
			}
		}
	}

	/** Clear only blocks completely covered by a repaint, before window partitioning. */
	void Clear(int left, int top, int right, int bottom)
	{
		left = std::max(0, left);
		top = std::max(0, top);
		right = std::min(this->width, right);
		bottom = std::min(this->height, bottom);
		if (left >= right || top >= bottom) return;
		if (left == 0 && top == 0 && right == this->width && bottom == this->height) this->have_dependencies = false;
		const int end_x = right == this->width ? this->columns : right / BLOCK_WIDTH;
		const int end_y = bottom == this->height ? this->rows : bottom / BLOCK_HEIGHT;
		for (int x = (left + BLOCK_WIDTH - 1) / BLOCK_WIDTH; x < end_x; x++) {
			for (int y = (top + BLOCK_HEIGHT - 1) / BLOCK_HEIGHT; y < end_y; y++) {
				this->dependencies[static_cast<size_t>(x) * this->rows + y] = 0;
			}
		}
	}

	/** Move dependencies with copied pixels, preserving masks outside the copied rectangle. */
	void Scroll(int left, int top, int width, int height, int dx, int dy)
	{
		if (!this->have_dependencies || (dx == 0 && dy == 0)) return;
		const int right = std::min(this->width, left + width);
		const int bottom = std::min(this->height, top + height);
		left = std::max(0, left);
		top = std::max(0, top);
		const int source_left = std::max(left, left - dx);
		const int source_top = std::max(top, top - dy);
		const int source_right = std::min(right, right - dx);
		const int source_bottom = std::min(bottom, bottom - dy);
		if (source_left >= source_right || source_top >= source_bottom) return;
		this->scroll_source = this->dependencies;
		this->Clear(source_left + dx, source_top + dy, source_right + dx, source_bottom + dy);
		for (int x = source_left / BLOCK_WIDTH; x < (source_right + BLOCK_WIDTH - 1) / BLOCK_WIDTH; ++x) {
			for (int y = source_top / BLOCK_HEIGHT; y < (source_bottom + BLOCK_HEIGHT - 1) / BLOCK_HEIGHT; ++y) {
				const uint32_t mask = this->scroll_source[static_cast<size_t>(x) * this->rows + y];
				this->Register(std::max(source_left, x * BLOCK_WIDTH) + dx,
						std::max(source_top, y * BLOCK_HEIGHT) + dy,
						std::min(source_right, (x + 1) * BLOCK_WIDTH) + dx,
						std::min(source_bottom, (y + 1) * BLOCK_HEIGHT) + dy, mask);
			}
		}
	}

	/** Visit only blocks affected by the changed animated palette entries. */
	template <typename Callback>
	void ForEachChanged(uint32_t changed, const Callback &callback) const
	{
		if (changed == 0) return;
		for (int x = 0; x < this->columns; x++) {
			for (int y = 0; y < this->rows; y++) {
				if ((this->dependencies[static_cast<size_t>(x) * this->rows + y] & changed) == 0) continue;
				callback(x * BLOCK_WIDTH, y * BLOCK_HEIGHT,
						std::min(this->width, (x + 1) * BLOCK_WIDTH), std::min(this->height, (y + 1) * BLOCK_HEIGHT));
			}
		}
	}
};

/** Mark fractional pixels dirty after changes to the animation palette. */
void InvalidateSmoothZoomPalette(uint32_t changed);

#endif /* SMOOTH_ZOOM_PALETTE_HPP */
