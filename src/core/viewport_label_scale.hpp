/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file viewport_label_scale.hpp Continuous, readability-bounded map-label scaling. */
#ifndef VIEWPORT_LABEL_SCALE_HPP
#define VIEWPORT_LABEL_SCALE_HPP
#include <algorithm>
#include <cmath>

inline double GetViewportLabelScale(double step, double normal_step)
{
	return std::clamp(std::pow(normal_step / step, 0.25), 0.75, 1.5);
}
inline int GetViewportLabelExtent(int pixels, double scale)
{
	return std::max(1, static_cast<int>(std::ceil(pixels * scale)));
}
/** Black/white raster passes recover coverage without assuming glyph alpha semantics. */
template <typename Pixel>
Pixel CompositeViewportLabelPixel(const Pixel &black, const Pixel &white, const Pixel &background)
{
	const int inverse_alpha = std::clamp(((int(white.r) - black.r) + (int(white.g) - black.g) + (int(white.b) - black.b) + 1) / 3, 0, 255);
	return Pixel(std::min(255, black.r + (background.r * inverse_alpha + 127) / 255),
			std::min(255, black.g + (background.g * inverse_alpha + 127) / 255),
			std::min(255, black.b + (background.b * inverse_alpha + 127) / 255));
}
#endif
