/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file viewport_flight.cpp Numerical regressions for destination camera navigation. */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../core/viewport_flight.hpp"

#include "../safeguards.h"

TEST_CASE("Camera flight pans and zooms together and arrives exactly", "[smoothzoom][flight]")
{
	ViewportFlight flight({100, 200, 4}, {12100, -5800, 4}, 1, 32, 1000, 100, 350, 2);
	CHECK(flight.GetPose().x == 100);
	CHECK(flight.GetPose().y == 200);
	CHECK(flight.GetPose().step == 4);
	flight.Advance(300);
	CHECK(flight.GetPose().x > 100);
	CHECK(flight.GetPose().x < 12100);
	CHECK(flight.GetPose().y < 200);
	CHECK(flight.GetPose().step > 4);
	flight.Advance(600);
	CHECK(flight.GetPose().x > 100);
	CHECK(flight.GetPose().x < 12100);
	CHECK(flight.GetPose().step > 4);
	flight.Advance(10000);
	CHECK_FALSE(flight.IsActive());
	CHECK(flight.GetPose().x == 12100);
	CHECK(flight.GetPose().y == -5800);
	CHECK(flight.GetPose().step == 4);
}

TEST_CASE("Camera flight respects zoom bounds and extra zoom-out limits", "[smoothzoom][flight]")
{
	const double extra = GENERATE(0.0, 1.0, 2.0);
	const double start = GENERATE(1.0, 4.0, 16.0);
	const double arrival = GENERATE(1.0, 4.0, 32.0);
	ViewportFlight flight({0, 0, start}, {90000, 30000, arrival}, 1, 32, 640, 100, 350, extra);
	bool quality_tail = false;
	while (flight.IsActive()) {
		flight.Advance(16);
		const auto pose = flight.GetPose();
		CHECK(pose.step >= 1);
		CHECK(pose.step <= 32);
		CHECK(pose.step <= std::max(start, arrival) * std::exp2(extra));
		CHECK(pose.x >= 0);
		CHECK(pose.x <= 90000);
		if (!flight.UseFastSampling(640) && flight.IsActive()) quality_tail = true;
		if (quality_tail) CHECK_FALSE(flight.UseFastSampling(640));
	}
	CHECK(flight.GetPose().step == arrival);
	CHECK_FALSE(flight.UseFastSampling(640));
}

TEST_CASE("Flight continues panning on both sides of the zoom apex", "[smoothzoom][flight]")
{
	ViewportFlight flight({0, 0, 4}, {12000, 6000, 8}, 1, 32, 1000, 100, 350, 2);
	const auto quarter = static_cast<unsigned int>(flight.GetDuration() / 4);
	flight.Advance(quarter);
	const auto departure = flight.GetPose();
	CHECK(departure.x > 0);
	CHECK(departure.x < 6000);
	CHECK(departure.step > 4);
	flight.Advance(quarter);
	const auto apex = flight.GetPose();
	CHECK(apex.x > departure.x);
	CHECK(apex.step > departure.step);
	flight.Advance(quarter);
	const auto approach = flight.GetPose();
	CHECK(approach.x > apex.x);
	CHECK(approach.x < 12000);
	CHECK(approach.step < apex.step);
	CHECK(approach.step > 8);
}

TEST_CASE("Nearby camera navigation avoids unnecessary zoom-out", "[smoothzoom][flight]")
{
	ViewportFlight flight({0, 0, 4}, {200, 100, 4}, 1, 32, 1280, 100, 350, 2);
	CHECK(flight.GetDuration() == 150);
	while (flight.IsActive()) {
		flight.Advance(16);
		CHECK(flight.GetPose().step == 4);
	}
}

TEST_CASE("Unequal flight endpoints zoom throughout without extra pull-back", "[smoothzoom][flight]")
{
	ViewportFlight flight({0, 0, 4}, {12000, 6000, 8}, 1, 32, 1000, 100, 350, 0);
	const auto quarter = static_cast<unsigned int>(flight.GetDuration() / 4);
	double previous = 4;
	for (int i = 0; i < 3; ++i) {
		flight.Advance(quarter);
		CHECK(flight.GetPose().step > previous);
		CHECK(flight.GetPose().step < 8);
		previous = flight.GetPose().step;
	}
}

TEST_CASE("Flight pan and zoom timing can be tuned independently", "[smoothzoom][flight]")
{
	const ViewportFlight::Pose start{0, 0, 4}, end{12000, 3000, 4};
	ViewportFlight normal(start, end, 1, 32, 1280, 100, 350, 2);
	ViewportFlight fast_pan(start, end, 1, 32, 1280, 200, 350, 2);
	ViewportFlight slow_zoom(start, end, 1, 32, 1280, 100, 700, 2);
	CHECK(fast_pan.GetDuration() < normal.GetDuration());
	CHECK(slow_zoom.GetDuration() == Approx(normal.GetDuration() + 700));
	ViewportFlight partitioned = normal;
	normal.Advance(320);
	for (int i = 0; i < 20; ++i) partitioned.Advance(16);
	CHECK(partitioned.GetPose().x == normal.GetPose().x);
	CHECK(partitioned.GetPose().y == normal.GetPose().y);
	CHECK(partitioned.GetPose().step == normal.GetPose().step);
}

TEST_CASE("Flight town fitting selects traditional levels and honours limits", "[smoothzoom][flight]")
{
	CHECK(GetNavigationArrivalStep(1600, 800, 1000, 600, 4, 1, 32) == 4);
	CHECK(GetNavigationArrivalStep(4000, 800, 1000, 600, 4, 1, 32) == 8);
	CHECK(GetNavigationArrivalStep(800, 5000, 1000, 600, 4, 1, 32) == 16);
	CHECK(GetNavigationArrivalStep(100000, 100000, 1000, 600, 4, 1, 16) == 16);
	CHECK(GetNavigationArrivalStep(1600, 800, 1000, 600, 4, 8, 32) == 8);
	ViewportFlight restricted({0, 0, 0.5}, {3000, 1000, 64}, 2, 16, 640, 100, 350, 2);
	CHECK(restricted.GetPose().step == 2);
	restricted.Advance(10000);
	CHECK(restricted.GetPose().step == 16);
}
