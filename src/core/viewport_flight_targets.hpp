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
	Town, ///< Town directory or town location.
	Vehicle, ///< Vehicle location control.
	Station, ///< Station location control.
	Industry, ///< Industry location control.
	News, ///< News or event destination.
	Sign, ///< Sign-list destination.
	SmallMap, ///< Small-map click.
	Company, ///< Company headquarters or location.
	Depot, ///< Depot destination.
	Waypoint, ///< Waypoint destination.
	Order, ///< Order-list location.
	Scenario, ///< Scenario or editor destination.
	Subsidy, ///< Subsidy endpoint.
	Script, ///< Script-requested location.
	Other, ///< Uncategorised map destination.
};

/**
 * Each category needs its own switch so disabling one does not enable a fallback flight.
 * @tparam Settings Settings type exposing flight switch members.
 * @param settings Client camera switches.
 * @param target Category of the initiating navigation action.
 * @return True when both the master and category switches allow a flight.
 */
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
		case ViewportFlightTarget::Other: return settings.fly_towns;
	}
	return false;
}

#endif /* VIEWPORT_FLIGHT_TARGETS_HPP */
