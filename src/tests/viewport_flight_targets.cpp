/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file viewport_flight_targets.cpp Verify independent navigation permissions. */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../settings_type.h"
#include "../core/viewport_flight_targets.hpp"

#include "../safeguards.h"

TEST_CASE("Flight categories remain independent under the master switch", "[smoothzoom][flight]")
{
	const std::pair<ViewportFlightTarget, bool GUISettings::*> categories[] = {
		{ViewportFlightTarget::Town, &GUISettings::fly_towns},
		{ViewportFlightTarget::Vehicle, &GUISettings::fly_vehicles},
		{ViewportFlightTarget::Station, &GUISettings::fly_stations},
		{ViewportFlightTarget::Industry, &GUISettings::fly_industries},
		{ViewportFlightTarget::News, &GUISettings::fly_news},
		{ViewportFlightTarget::Sign, &GUISettings::fly_signs},
		{ViewportFlightTarget::SmallMap, &GUISettings::fly_smallmap},
		{ViewportFlightTarget::Company, &GUISettings::fly_companies},
		{ViewportFlightTarget::Depot, &GUISettings::fly_depots},
		{ViewportFlightTarget::Waypoint, &GUISettings::fly_waypoints},
		{ViewportFlightTarget::Order, &GUISettings::fly_orders},
		{ViewportFlightTarget::Scenario, &GUISettings::fly_scenarios},
		{ViewportFlightTarget::Subsidy, &GUISettings::fly_subsidies},
		{ViewportFlightTarget::Script, &GUISettings::fly_scripts},
		{ViewportFlightTarget::Other, &GUISettings::fly_other_navigation},
	};
	GUISettings settings{};
	settings.fly_navigation = true;
	for (const auto &[enabled_target, enabled_flag] : categories) {
		settings.*enabled_flag = true;
		for (const auto &[target, flag] : categories) {
			CHECK(IsViewportFlightEnabled(settings, target) == (target == enabled_target));
		}
		settings.fly_navigation = false;
		CHECK_FALSE(IsViewportFlightEnabled(settings, enabled_target));
		settings.fly_navigation = true;
		settings.*enabled_flag = false;
		CHECK_FALSE(IsViewportFlightEnabled(settings, enabled_target));
	}
	for (const auto &[target, flag] : categories) settings.*flag = true;
	settings.fly_navigation = false;
	for (const auto &[target, flag] : categories) CHECK_FALSE(IsViewportFlightEnabled(settings, target));
	settings.fly_navigation = true;
	settings.fly_news = false;
	CHECK_FALSE(IsViewportFlightEnabled(settings, ViewportFlightTarget::News));
	CHECK(IsViewportFlightEnabled(settings, ViewportFlightTarget::Vehicle));
	CHECK(IsViewportFlightEnabled(settings, ViewportFlightTarget::Other));
}
