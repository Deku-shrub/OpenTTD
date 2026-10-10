/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file viewport_flight_targets.hpp Independent navigation flight permissions. */
#ifndef VIEWPORT_FLIGHT_TARGETS_HPP
#define VIEWPORT_FLIGHT_TARGETS_HPP

/** Category of the navigation action, independent of its arrival zoom. */
enum class ViewportFlightTarget {
	Town, Vehicle, Station, Industry, News, Sign, SmallMap, Company, Depot,
	Waypoint, Order, Scenario, Subsidy, Script, Other,
};

/** The master switch gates each category; other destinations are not a fallback permission. */
template <typename Settings>
bool IsViewportFlightEnabled(const Settings &settings, ViewportFlightTarget target)
{
	if (!settings.fly_navigation) return false;
	switch (target) {
		case ViewportFlightTarget::Town: return settings.fly_towns;
		case ViewportFlightTarget::Vehicle: return settings.fly_vehicles;
		case ViewportFlightTarget::Station: return settings.fly_stations;
		case ViewportFlightTarget::Industry: return settings.fly_industries;
		case ViewportFlightTarget::News: return settings.fly_news;
		case ViewportFlightTarget::Sign: return settings.fly_signs;
		case ViewportFlightTarget::SmallMap: return settings.fly_smallmap;
		case ViewportFlightTarget::Company: return settings.fly_companies;
		case ViewportFlightTarget::Depot: return settings.fly_depots;
		case ViewportFlightTarget::Waypoint: return settings.fly_waypoints;
		case ViewportFlightTarget::Order: return settings.fly_orders;
		case ViewportFlightTarget::Scenario: return settings.fly_scenarios;
		case ViewportFlightTarget::Subsidy: return settings.fly_subsidies;
		case ViewportFlightTarget::Script: return settings.fly_scripts;
		case ViewportFlightTarget::Other: return settings.fly_other_navigation;
	}
	return false;
}

#endif /* VIEWPORT_FLIGHT_TARGETS_HPP */
