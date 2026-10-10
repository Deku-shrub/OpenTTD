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
#include "../settings_internal.h"
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
		{ViewportFlightTarget::Other, &GUISettings::fly_towns},
	};
	GUISettings settings{};
	settings.fly_navigation = true;
	for (const auto &[enabled_target, enabled_flag] : categories) {
		settings.*enabled_flag = true;
		for (const auto &[target, flag] : categories) {
			CHECK(IsViewportFlightEnabled(settings, target) == (flag == enabled_flag));
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

TEST_CASE("Enabling the main flight switch allows all default destination categories", "[smoothzoom][flight]")
{
	const auto default_value = [](std::string_view name) {
		const SettingDesc *setting = GetSettingFromName(name);
		REQUIRE(setting != nullptr);
		REQUIRE(setting->IsIntSetting());
		return static_cast<const IntSettingDesc *>(setting)->GetDefaultValue();
	};
	CHECK(default_value("gui.fly_navigation") == 0);
	GUISettings settings{};
	settings.fly_navigation = true;
	settings.fly_towns = default_value("gui.fly_towns");
	settings.fly_vehicles = default_value("gui.fly_vehicles");
	settings.fly_stations = default_value("gui.fly_stations");
	settings.fly_industries = default_value("gui.fly_industries");
	settings.fly_news = default_value("gui.fly_news");
	settings.fly_signs = default_value("gui.fly_signs");
	settings.fly_smallmap = default_value("gui.fly_smallmap");
	settings.fly_companies = default_value("gui.fly_companies");
	settings.fly_depots = default_value("gui.fly_depots");
	settings.fly_waypoints = default_value("gui.fly_waypoints");
	settings.fly_orders = default_value("gui.fly_orders");
	settings.fly_scenarios = default_value("gui.fly_scenarios");
	settings.fly_subsidies = default_value("gui.fly_subsidies");
	settings.fly_scripts = default_value("gui.fly_scripts");
	for (auto target : {ViewportFlightTarget::Town, ViewportFlightTarget::Vehicle,
			ViewportFlightTarget::Station, ViewportFlightTarget::Industry, ViewportFlightTarget::News,
			ViewportFlightTarget::Sign, ViewportFlightTarget::SmallMap, ViewportFlightTarget::Company,
			ViewportFlightTarget::Depot, ViewportFlightTarget::Waypoint, ViewportFlightTarget::Order,
			ViewportFlightTarget::Scenario, ViewportFlightTarget::Subsidy, ViewportFlightTarget::Script,
			ViewportFlightTarget::Other}) {
		CHECK(IsViewportFlightEnabled(settings, target));
	}
	settings.fly_vehicles = false;
	CHECK_FALSE(IsViewportFlightEnabled(settings, ViewportFlightTarget::Vehicle));
	CHECK(IsViewportFlightEnabled(settings, ViewportFlightTarget::Town));
	CHECK(default_value("gui.fly_town_zoom") == 0);
}
