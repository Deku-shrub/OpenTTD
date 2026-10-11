/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file smooth_zoom_scale.hpp Portable, locale-independent console zoom parsing. */
#ifndef SMOOTH_ZOOM_SCALE_HPP
#define SMOOTH_ZOOM_SCALE_HPP

#include <cmath>
#include <locale>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

/**
 * Parse a complete nonnegative decimal scale; zero selects traditional zoom.
 * @param text Entire console argument using a decimal point.
 * @return Finite nonnegative scale, or nullopt for invalid input.
 */
inline std::optional<float> ParseSmoothZoomScale(std::string_view text)
{
	if (text.empty() || text.front() == '+' || text.find_first_not_of("0123456789.eE+-") != std::string_view::npos) return std::nullopt;
	/* Floating-point from_chars is unavailable at our minimum macOS target.
	 * Classic locale keeps console commands independent of the user's locale. */
	std::istringstream input{std::string(text)};
	input.imbue(std::locale::classic());
	float scale;
	input >> std::noskipws >> scale;
	if (input.fail() || !input.eof() || !std::isfinite(scale) || scale < 0) return std::nullopt;
	return scale;
}

#endif /* SMOOTH_ZOOM_SCALE_HPP */
