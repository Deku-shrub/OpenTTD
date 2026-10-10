/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file smooth_zoom_controls.cpp Tests for wheel zoom limits and cursor anchoring. */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../zoom_func.h"
#include "../core/smooth_zoom_palette.hpp"
#include "../core/smooth_zoom_motion.hpp"

#include "../safeguards.h"

TEST_CASE("Zoom motion settles exactly without overshooting a single target", "[smoothzoom]")
{
	for (const auto [start, target] : {std::pair{4.0, 2.0}, std::pair{2.0, 4.0}, std::pair{3.7, 1.0}}) {
		SmoothZoomMotion motion(start);
		motion.Retarget(target);
		double previous = start;
		for (uint elapsed = 0; elapsed < 500; elapsed += 8) {
			motion.Advance(8);
			const double step = motion.GetStep();
			CHECK(step >= std::min(start, target));
			CHECK(step <= std::max(start, target));
			CHECK((target < start ? step <= previous : step >= previous));
			previous = step;
		}
		CHECK(motion.GetStep() == target);
		CHECK_FALSE(motion.IsActive());
	}
}

TEST_CASE("Zoom motion preserves velocity when wheel targets change", "[smoothzoom]")
{
	SmoothZoomMotion motion(4);
	motion.Retarget(2);
	motion.Advance(32);
	const double position = motion.GetStep();
	const double velocity = motion.GetVelocity();
	REQUIRE(velocity < 0);
	motion.Retarget(1);
	CHECK(motion.GetStep() == position);
	CHECK(motion.GetVelocity() == velocity);
	motion.Advance(16);
	CHECK(motion.GetStep() < position);
	const double reversing_velocity = motion.GetVelocity();
	motion.Retarget(8);
	CHECK(motion.GetVelocity() == reversing_velocity);
	for (int i = 0; i < 100; ++i) {
		motion.Advance(8);
		CHECK(motion.GetStep() >= 1);
		CHECK(motion.GetStep() <= 8);
	}
	CHECK(motion.GetStep() == 8);
}

TEST_CASE("Zoom restores settled filtering before the visible motion finishes", "[smoothzoom]")
{
	for (double extent : {640.0, 1280.0, 3840.0}) {
		SmoothZoomMotion motion(4);
		motion.Retarget(4 * std::exp2(0.25));
		REQUIRE(motion.UseFastSampling(extent));
		bool quality_tail = false;
		uint elapsed = 0;
		for (; elapsed < 600 && motion.IsActive(); elapsed += 16) {
			motion.Advance(16, extent);
			if (!motion.UseFastSampling(extent) && motion.IsActive()) quality_tail = true;
			if (quality_tail) CHECK_FALSE(motion.UseFastSampling(extent));
		}
		CHECK(quality_tail);
		CHECK_FALSE(motion.IsActive());
		CHECK(elapsed <= 400);
		CHECK(motion.GetStep() == motion.GetTargetStep());
		motion.Retarget(4);
		CHECK(motion.UseFastSampling(extent));
	}
}

TEST_CASE("Zoom quality transition respects momentum when passing a new target", "[smoothzoom]")
{
	SmoothZoomMotion motion(4);
	motion.Retarget(2);
	motion.Advance(32, 3840);
	motion.Retarget(motion.GetStep());
	CHECK(motion.UseFastSampling(3840));
	motion.Advance(16, 3840);
	CHECK(motion.IsActive());
	CHECK(motion.UseFastSampling(3840));
}

TEST_CASE("Zoom motion is independent of display frame partitions", "[smoothzoom]")
{
	SmoothZoomMotion single(4);
	SmoothZoomMotion frames(4);
	single.Retarget(2);
	frames.Retarget(2);
	single.Advance(96);
	for (int i = 0; i < 6; ++i) frames.Advance(16);
	CHECK(frames.GetStep() == Approx(single.GetStep()).epsilon(1e-12));
	CHECK(frames.GetVelocity() == Approx(single.GetVelocity()).epsilon(1e-12));
	frames.Advance(1000);
	CHECK(frames.GetStep() == 2);
	CHECK_FALSE(frames.IsActive());
}

TEST_CASE("Palette repaint tracking preserves partial block dependencies", "[smoothzoom]")
{
	SmoothZoomPaletteTracker tracker;
	tracker.Resize(70, 10);
	auto count = [&](uint32_t changed) {
		int blocks = 0;
		tracker.ForEachChanged(changed, [&](int, int, int, int) { ++blocks; });
		return blocks;
	};
	tracker.Register(0, 0, 70, 10, 1);
	tracker.Register(64, 8, 70, 10, 2);
	CHECK(count(1) == 4);
	CHECK(count(2) == 1);
	CHECK(count(4) == 0);
	tracker.Clear(0, 0, 32, 8);
	CHECK(count(1) == 4);
	tracker.Clear(0, 0, 64, 8);
	CHECK(count(1) == 3);
	tracker.Clear(64, 8, 70, 10);
	CHECK(count(1) == 2);
	CHECK(count(2) == 0);
	tracker.Resize(128, 16);
	CHECK(count(1) == 0);
}

TEST_CASE("Four smooth wheel notches span one integer zoom level", "[smoothzoom]")
{
	CHECK(GetSmoothZoomStep(4, true, ZoomLevel::Min, ZoomLevel::Max, 4) == 2);
	CHECK(GetSmoothZoomStep(4, false, ZoomLevel::Min, ZoomLevel::Max, 4) == 8);
	CHECK(GetSmoothZoomStep(4, true, ZoomLevel::Min, ZoomLevel::Max, 100) == 1);
	CHECK(GetSmoothZoomStep(4, false, ZoomLevel::Min, ZoomLevel::Max, 100) == 32);
	for (int level = 1; level < to_underlying(ZoomLevel::Max); ++level) {
		const double initial = ScaleByZoom(1, static_cast<ZoomLevel>(level));
		double step = initial;
		for (int i = 0; i < 4; ++i) step = GetSmoothZoomStep(step, true, ZoomLevel::Min, ZoomLevel::Max);
		CHECK(step == Approx(initial / 2));
		for (int i = 0; i < 4; ++i) step = GetSmoothZoomStep(step, false, ZoomLevel::Min, ZoomLevel::Max);
		CHECK(step == Approx(initial));
	}
}

TEST_CASE("Smooth wheel respects restricted client zoom limits", "[smoothzoom]")
{
	double step = 4;
	for (int i = 0; i < 100; ++i) step = GetSmoothZoomStep(step, true, ZoomLevel::In2x, ZoomLevel::Out4x);
	CHECK(step == 2);
	for (int i = 0; i < 100; ++i) step = GetSmoothZoomStep(step, false, ZoomLevel::In2x, ZoomLevel::Out4x);
	CHECK(step == 16);
}

TEST_CASE("Cursor anchor preserves world position to half a virtual pixel", "[smoothzoom]")
{
	for (const int cursor : {0, 1, 317, 1279}) {
		for (const double old_step : {1.0, 1.3, 3.7, 8.0, 31.0}) {
			for (const bool in : {false, true}) {
				const double new_step = GetSmoothZoomStep(old_step, in, ZoomLevel::Min, ZoomLevel::Max);
				for (const int origin : {-12345, 0, 12345}) {
					const double before = origin + cursor * old_step;
					const double after = origin + GetZoomAnchorOffset(cursor, old_step, new_step) + cursor * new_step;
					CHECK(std::abs(after - before) <= 0.500000001);
				}
			}
		}
	}
}

TEST_CASE("Zoom buttons choose adjacent integer levels from fractional scales", "[smoothzoom]")
{
	CHECK(GetNextIntegerZoom(3, true, ZoomLevel::Min, ZoomLevel::Max) == ZoomLevel::In2x);
	CHECK(GetNextIntegerZoom(3, false, ZoomLevel::Min, ZoomLevel::Max) == ZoomLevel::Normal);
	CHECK(GetNextIntegerZoom(4, true, ZoomLevel::Min, ZoomLevel::Max) == ZoomLevel::In2x);
	CHECK(GetNextIntegerZoom(4, false, ZoomLevel::Min, ZoomLevel::Max) == ZoomLevel::Out2x);
	CHECK(GetNextIntegerZoom(1.01, true, ZoomLevel::Min, ZoomLevel::Max) == ZoomLevel::Min);
	CHECK(GetNextIntegerZoom(31.99, false, ZoomLevel::Min, ZoomLevel::Max) == ZoomLevel::Max);
	CHECK(GetNextIntegerZoom(2, true, ZoomLevel::In2x, ZoomLevel::Out4x) == ZoomLevel::In2x);
	CHECK(GetNextIntegerZoom(16, false, ZoomLevel::In2x, ZoomLevel::Out4x) == ZoomLevel::Out4x);
}
