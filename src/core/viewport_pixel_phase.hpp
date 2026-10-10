/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file viewport_pixel_phase.hpp Pixel-aligned translation at a fixed fractional scale. */
#ifndef VIEWPORT_PIXEL_PHASE_HPP
#define VIEWPORT_PIXEL_PHASE_HPP

#include <cmath>
#include <utility>

/** Preserve the source sampling phase when scrolling already-rendered pixels. */
struct ViewportPixelPhase {
	double left = 0;
	double top = 0;
	float step = 0;

	void Reset(int x, int y, float new_step)
	{
		this->left = x;
		this->top = y;
		this->step = new_step;
	}

	/** Return the whole screen-pixel translation; retain subpixel input in the desired camera coordinates. */
	std::pair<int, int> Move(int x, int y)
	{
		/* A half-open rounding interval keeps an exact half-pixel tie stable
		 * when the same desired coordinate is submitted on later frames. */
		const int dx = static_cast<int>(std::floor((this->left - x) / this->step + 0.5));
		const int dy = static_cast<int>(std::floor((this->top - y) / this->step + 0.5));
		this->left -= dx * static_cast<double>(this->step);
		this->top -= dy * static_cast<double>(this->step);
		return {dx, dy};
	}
};

#endif /* VIEWPORT_PIXEL_PHASE_HPP */
