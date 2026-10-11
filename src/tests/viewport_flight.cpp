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
	ViewportFlight flight({100, 200, 4}, {12100, -5800, 4}, 1, 32, 1000, 200, 100, 2);
	CHECK(flight.GetPose().x == 100);
	CHECK(flight.GetPose().y == 200);
	CHECK(flight.GetPose().step == 4);
	flight.Advance(300);
	CHECK(flight.GetPose().x > 100);
	CHECK(flight.GetPose().x < 12100);
	CHECK(flight.GetPose().y < 200);
	CHECK(flight.GetPose().step > 4);
	flight.Advance(100);
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
	ViewportFlight flight({0, 0, start}, {90000, 30000, arrival}, 1, 32, 640, 100000, 100, extra);
	while (flight.IsActive()) {
		flight.Advance(16);
		const auto pose = flight.GetPose();
		CHECK(pose.step >= 1);
		CHECK(pose.step <= 32);
		CHECK(pose.step <= std::max(start, arrival) * std::exp2(extra));
		CHECK(pose.x >= 0);
		CHECK(pose.x <= 90000);
	}
	CHECK(flight.GetPose().step == arrival);
}

TEST_CASE("Flight continues panning on both sides of the zoom apex", "[smoothzoom][flight]")
{
	ViewportFlight flight({0, 0, 4}, {12000, 6000, 8}, 1, 32, 1000, 200, 100, 2);
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
	ViewportFlight flight({0, 0, 4}, {200, 100, 4}, 1, 32, 1280, 200, 100, 2);
	CHECK(flight.GetDuration() == 300);
	while (flight.IsActive()) {
		flight.Advance(16);
		CHECK(flight.GetPose().step == 4);
	}
}

TEST_CASE("Unequal flight endpoints zoom throughout without extra pull-back", "[smoothzoom][flight]")
{
	ViewportFlight flight({0, 0, 4}, {12000, 6000, 8}, 1, 32, 1000, 1000, 100, 0);
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
	ViewportFlight normal(start, end, 1, 32, 1280, 200, 100, 2);
	ViewportFlight fast_pan(start, end, 1, 32, 1280, 400, 100, 2);
	ViewportFlight slow_zoom(start, end, 1, 32, 1280, 200, 125, 2);
	CHECK(fast_pan.GetDuration() < normal.GetDuration());
	CHECK(slow_zoom.GetDuration() == Approx(normal.GetDuration() * 1.25));
	ViewportFlight partitioned = normal;
	normal.Advance(320);
	for (int i = 0; i < 20; ++i) partitioned.Advance(16);
	CHECK(partitioned.GetPose().x == normal.GetPose().x);
	CHECK(partitioned.GetPose().y == normal.GetPose().y);
	CHECK(partitioned.GetPose().step == normal.GetPose().step);
}

TEST_CASE("Flight destination fitting selects traditional levels and honours limits", "[smoothzoom][flight]")
{
	CHECK(GetNavigationArrivalStep(1600, 800, 1000, 600, 4, 1, 32) == 4);
	CHECK(GetNavigationArrivalStep(4000, 800, 1000, 600, 4, 1, 32) == 8);
	CHECK(GetNavigationArrivalStep(800, 5000, 1000, 600, 4, 1, 32) == 16);
	CHECK(GetNavigationArrivalStep(100000, 100000, 1000, 600, 4, 1, 16) == 16);
	CHECK(GetNavigationArrivalStep(1600, 800, 1000, 600, 4, 8, 32) == 8);
	ViewportFlight restricted({0, 0, 0.5}, {3000, 1000, 64}, 2, 16, 640, 200, 100, 2);
	CHECK(restricted.GetPose().step == 2);
	restricted.Advance(10000);
	CHECK(restricted.GetPose().step == 16);
}

TEST_CASE("Perceptual flights have logarithmic distance growth and a bounded duration", "[smoothzoom][flight]")
{
	ViewportFlight medium({0, 0, 4}, {6000, 0, 4}, 1, 128, 1000, 120, 100, 5);
	ViewportFlight farther({0, 0, 4}, {12000, 0, 4}, 1, 128, 1000, 120, 100, 5);
	CHECK(medium.GetPathLength() > 0);
	CHECK(farther.GetPathLength() > medium.GetPathLength());
	CHECK(farther.GetPathLength() < medium.GetPathLength() * 2);
	CHECK(medium.GetDuration() >= 300);
	CHECK(medium.GetDuration() <= 2200);
	ViewportFlight cross_map({0, 0, 1}, {10000000, 0, 1}, 1, 32, 640, 120, 100, 0);
	CHECK(cross_map.GetDuration() == 2200);
	CHECK(cross_map.IsActive());
	cross_map.Advance(2200);
	CHECK_FALSE(cross_map.IsActive());
	CHECK(cross_map.GetPose().x == 10000000);
	CHECK(cross_map.GetPose().step == 1);
}

TEST_CASE("Pure zoom and identical flight states remain finite and preserve fractional arrival", "[smoothzoom][flight]")
{
	for (double arrival : {1.0, 3.3, 8.0}) {
		ViewportFlight flight({100, 200, 4}, {100, 200, arrival}, 1, 32, 960, 120, 100, 2);
		while (flight.IsActive()) {
			flight.Advance(17);
			const auto pose = flight.GetPose();
			CHECK(pose.x == 100);
			CHECK(pose.y == 200);
			CHECK(std::isfinite(pose.step));
			CHECK(pose.step >= std::min(4.0, arrival));
			CHECK(pose.step <= std::max(4.0, arrival));
		}
		CHECK(flight.GetPose().step == arrival);
	}
}

TEST_CASE("Short hops avoid bobbing across the full visible screen", "[smoothzoom][flight]")
{
	ViewportFlight flight({0, 0, 4}, {3900, 0, 4}, 1, 32, 1000, 120, 100, 2);
	while (flight.IsActive()) {
		flight.Advance(16);
		CHECK(flight.GetPose().step == 4);
	}
}

TEST_CASE("Curvature controls flight height and the cap flattens without a travel plateau", "[smoothzoom][flight]")
{
	ViewportFlight flat({0, 0, 4}, {8000, 0, 4}, 1, 32, 1000, 400, 100, 2, 0.7);
	ViewportFlight high({0, 0, 4}, {8000, 0, 4}, 1, 32, 1000, 400, 100, 2, 2.0);
	flat.Advance(static_cast<unsigned int>(flat.GetDuration() / 2));
	high.Advance(static_cast<unsigned int>(high.GetDuration() / 2));
	CHECK(high.GetPose().step > flat.GetPose().step);
	ViewportFlight capped({0, 0, 4}, {20000, 0, 4}, 1, 8, 1000, 400, 100, 2);
	const auto quarter = static_cast<unsigned int>(capped.GetDuration() / 4);
	capped.Advance(quarter);
	const auto first = capped.GetPose();
	capped.Advance(quarter);
	const auto apex = capped.GetPose();
	capped.Advance(quarter);
	const auto last = capped.GetPose();
	CHECK(first.x < apex.x);
	CHECK(apex.x < last.x);
	CHECK(first.step < apex.step);
	CHECK(last.step < apex.step);
	CHECK(apex.step <= 8);
}

TEST_CASE("Moving vehicle arrival blends only in the final fifth without extending duration", "[smoothzoom][flight]")
{
	ViewportFlight flight({0, 0, 4}, {8000, 0, 4}, 1, 32, 1000, 200, 100, 2);
	flight.Advance(static_cast<unsigned int>(flight.GetDuration() * 0.75));
	CHECK(flight.GetPoseTowards(8100, 200).x == flight.GetPose().x);
	CHECK(flight.GetPoseTowards(8100, 200).y == 0);
	flight.Advance(static_cast<unsigned int>(flight.GetDuration() * 0.15));
	CHECK(flight.GetPoseTowards(8100, 200).x > flight.GetPose().x);
	CHECK(flight.GetPoseTowards(8100, 200).y > 0);
	CHECK(flight.GetPoseTowards(8100, 200).y < 200);
	flight.Advance(10000);
	CHECK(flight.GetPoseTowards(8100, 200).x == 8100);
	CHECK(flight.GetPoseTowards(8100, 200).y == 200);
	CHECK(flight.GetPoseTowards(8100, 200).step == 4);
}

TEST_CASE("Perceptual flight agrees with the published d3 zoom interpolation example", "[smoothzoom][flight]")
{
	ViewportFlight flight({30, 30, 40}, {135, 85, 60}, 1, 512, 1, 1000, 100, 4, std::sqrt(2.0));
	REQUIRE(flight.GetDuration() == 300);
	flight.Advance(150);
	CHECK(flight.GetPose().x == Approx(72));
	CHECK(flight.GetPose().y == Approx(52));
	CHECK(flight.GetPose().step == Approx(126.04761005270991));
}

TEST_CASE("Successive town destinations retain animated flights after arrival", "[smoothzoom][flight]")
{
	ViewportFlight::Pose pose{0, 10112, 4};
	for (const auto destination : {ViewportFlight::Pose{-5760, 8736, 4}, {-5632, 23424, 4}, {1408, 6560, 4}}) {
		ViewportFlight flight(pose, destination, 1, 32, 960, 120, 100, 2);
		REQUIRE(flight.IsActive());
		CHECK(flight.GetDuration() <= 2200);
		flight.Advance(static_cast<unsigned int>(flight.GetDuration() / 2));
		CHECK(flight.GetPose().x != destination.x);
		CHECK(flight.GetPose().step > 4);
		flight.Advance(3000);
		pose = flight.GetPose();
		CHECK(pose.x == destination.x);
		CHECK(pose.y == destination.y);
		CHECK(pose.step == 4);
	}
}

TEST_CASE("Destination fitting depends on object bounds rather than departure zoom", "[smoothzoom][flight]")
{
	for (const auto dimensions : {std::pair{1024.0, 640.0}, {4096.0, 1536.0}, {16384.0, 640.0}}) {
		const double arrival = GetNavigationArrivalStep(dimensions.first, dimensions.second, 1280, 800, 2, 1, 32);
		for (double start : {1.0, 3.076923, 16.0, 32.0}) {
			ViewportFlight flight({0, 0, start}, {6000, 3000, arrival}, 1, 32, 1280, 120, 100, 2);
			flight.Advance(3000);
			CHECK(flight.GetPose().step == arrival);
		}
	}
	CHECK(GetNavigationArrivalStep(1024, 640, 1280, 800, 2, 1, 32) == 2);
	CHECK(GetNavigationArrivalStep(4096, 1536, 1280, 800, 2, 1, 32) == 4);
	CHECK(GetNavigationArrivalStep(16384, 640, 1280, 800, 2, 1, 32) == 16);
	CHECK(GetNavigationArrivalStep(4096, 1536, 1280, 300, 2, 1, 32) == 8);
}

TEST_CASE("Vehicle fitting includes off-screen consist positions without sprite bounds", "[smoothzoom][flight]")
{
	struct Part {
		int x_pos, y_pos, z_pos;
		struct { int left, top, right, bottom; } coord;
		const Part *first = nullptr, *next = nullptr;
		const Part *First() const { return first; }
		const Part *Next() const { return next; }
	};
	Part front{100, 100, 0, {-1, 0, 0, 0}};
	Part tail{100, 600, 0, {-1, 0, 0, 0}};
	front.first = &front; front.next = &tail; tail.first = &front;
	const auto [width, height] = GetNavigationVehicleExtent(&front, 4, -1);
	CHECK(width == 8256);
	CHECK(height == 4320);
	CHECK(GetNavigationArrivalStep(width, height, 1280, 800, 2, 1, 32) == 16);
	front.next = nullptr;
	const auto [single_width, single_height] = GetNavigationVehicleExtent(&front, 4, -1);
	CHECK(single_width == 1024);
	CHECK(single_height == 640);
	CHECK(GetNavigationArrivalStep(single_width, single_height, 1280, 800, 2, 1, 32) == 2);
}

TEST_CASE("Fractional flight lands before an invisible tail and stays at its endpoint", "[smoothzoom][flight]")
{
	const double extent = GENERATE(640.0, 1280.0, 3840.0);
	const double arrival = GENERATE(4.0 / 1.3, 4.0 / 0.84);
	ViewportFlight flight({0, 0, 4}, {48000, 16000, arrival}, 1, 32, extent, 120, 100, 2);
	unsigned int elapsed = 0;
	while (flight.IsActive()) {
		auto uncompacted = flight;
		uncompacted.Advance(8);
		const auto previous = uncompacted.GetPose();
		elapsed += 8;
		flight.Advance(8, extent);
		if (!flight.IsActive()) {
			/* The omitted tail, excluding this frame's normal movement, is subpixel. */
			const double pan = std::hypot(48000 - previous.x, 16000 - previous.y) / previous.step;
			const double zoom = extent * std::abs(arrival / previous.step - 1);
			CHECK(pan + zoom <= 1);
		}
	}
	CHECK(elapsed < flight.GetDuration());
	CHECK(flight.GetPose().x == 48000);
	CHECK(flight.GetPose().y == 16000);
	CHECK(flight.GetPose().step == arrival);
	flight.Advance(1000, extent);
	CHECK(flight.GetPose().x == 48000);
	CHECK(flight.GetPose().step == arrival);
}
