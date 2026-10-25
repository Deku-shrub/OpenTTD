/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file viewport_camera.cpp Real-time viewport zoom and navigation animation. */

#include "stdafx.h"
#include "viewport_func.h"
#include "window_gui.h"
#include "settings_type.h"
#include "gfx_func.h"
#include "zoom_func.h"
#include "landscape.h"
#include "vehicle_base.h"
#include "openttd.h"
#include "debug.h"
#include "blitter/factory.hpp"
#include "core/math_func.hpp"
#include "core/smooth_zoom_motion.hpp"
#include "core/viewport_flight.hpp"
#include "linkgraph/linkgraph_gui.h"

#include <map>

#include "safeguards.h"

/** Per-window wheel easing, cursor anchoring and timing measurements. */
struct SmoothZoomAnimation {
	SmoothZoomMotion motion; ///< Wheel easing controller.
	double start; ///< Step when the current cursor anchor was captured.
	int anchor_x; ///< Cursor x within the viewport in screen pixels.
	int anchor_y; ///< Cursor y within the viewport in screen pixels.
	int origin_x; ///< Anchor origin in virtual x coordinates.
	int origin_y; ///< Anchor origin in virtual y coordinates.
	int last_scroll_x; ///< Previously displayed virtual x origin.
	int last_scroll_y; ///< Previously displayed virtual y origin.
	bool to_cursor; ///< Whether to preserve the cursor anchor.
	uint frames = 0; ///< Number of displayed transition frames.
	uint total_ms = 0; ///< Accumulated wheel transition time in milliseconds.
	uint longest_frame_ms = 0; ///< Longest display-frame interval in milliseconds.
};

static std::map<Window *, SmoothZoomAnimation> _smooth_zoom_animations; ///< Active wheel transitions by window.

/** Per-window navigation path, moving target and timing measurements. */
struct CameraFlight {
	ViewportFlight path; ///< Simultaneous navigation path.
	ViewportFlightTarget target; ///< Category that authorises this flight.
	VehicleID vehicle = VehicleID::Invalid(); ///< Optional exact moving vehicle target.
	uint elapsed_ms = 0; ///< Accumulated navigation time in milliseconds.
	uint frames = 0; ///< Number of displayed transition frames.
	uint longest_frame_ms = 0; ///< Longest display-frame interval in milliseconds.
	uint native_pan_frames = 0; ///< Frames panned at traditional scale.
	uint native_pan_ms = 0; ///< Time spent panning at traditional scale.
	uint longest_native_pan_ms = 0; ///< Longest traditional-scale pan interval in milliseconds.
};

static std::map<Window *, CameraFlight> _viewport_flights; ///< Active navigation flights by window.

/**
 * Cancel navigation at its currently displayed camera pose.
 * @param w Window whose flight should stop.
 */
void CancelViewportFlight(Window *w)
{
	if (_viewport_flights.erase(w) == 0) return;
	if (w != nullptr && w->viewport != nullptr) {
		w->viewport->dest_scrollpos_x = w->viewport->scrollpos_x;
		w->viewport->dest_scrollpos_y = w->viewport->scrollpos_y;
		w->SetDirty();
	}
}

/**
 * Cancel wheel easing and navigation without changing the displayed scale.
 * @param w Window whose camera animation should stop.
 */
void CancelViewportSmoothZoomAnimation(Window *w)
{
	CancelViewportFlight(w);
	_smooth_zoom_animations.erase(w);
}

static void UpdateViewportFlight(Window *w, uint delta_ms);

/**
 * Check navigation policy before fitting a destination or creating a flight.
 * @param w Destination window.
 * @param target Category of the navigation action.
 * @return True for a usable main viewport with enabled flight navigation.
 */
bool CanStartViewportFlight(const Window *w, ViewportFlightTarget target)
{
	return w != nullptr && w->window_class == WindowClass::MainWindow && w->viewport != nullptr && w->viewport->width > 0 && w->viewport->height > 0 &&
			_game_mode != GameMode::Menu && _settings_client.gui.smooth_scroll && IsViewportFlightEnabled(_settings_client.gui, target) &&
			BlitterFactory::GetCurrentBlitter()->SupportsSmoothZoom();
}

/**
 * Fit and track a vehicle without exposing animation state to GUI callers.
 * @param vehicle Selected vehicle or consist part.
 * @param w Destination window.
 * @param target Category authorising the navigation.
 * @return Whether the viewport destination changed.
 */
bool ScrollWindowToVehicle(const Vehicle &vehicle, Window *w, ViewportFlightTarget target)
{
	if (w == nullptr || w->viewport == nullptr) return false;
	const Vehicle *front = vehicle.GetMovingFront();
	double arrival_scale = 0;
	if (CanStartViewportFlight(w, target)) {
		const Viewport &vp = *w->viewport;
		const auto [width, height] = GetNavigationVehicleExtent(front, ZOOM_BASE, INVALID_COORD);
		arrival_scale = ZOOM_BASE / GetNavigationArrivalStep(width, height, vp.width, vp.height, ZOOM_BASE / 2.0,
				ScaleByZoom(1, _settings_client.gui.zoom_min), ScaleByZoom(1, _settings_client.gui.zoom_max));
	}
	const bool changed = ScrollWindowTo(front->x_pos, front->y_pos, front->z_pos, w, false, arrival_scale, target);
	auto it = _viewport_flights.find(w);
	if (it != _viewport_flights.end()) it->second.vehicle = front->index;
	return changed;
}

/**
 * Start an opt-in main-viewport flight; zero arrival scale preserves the current zoom.
 * @param w Main viewport window.
 * @param centre Destination centre in virtual coordinates.
 * @param arrival_scale Requested magnification; zero retains the intended landing zoom.
 * @param target Category of the navigation action.
 * @return True if enabled navigation is handled by a flight.
 */
bool TryStartViewportFlight(Window *w, Point centre, double arrival_scale, ViewportFlightTarget target)
{
	if (!CanStartViewportFlight(w, target)) return false;
	ViewportData &vp = *w->viewport;
	const double min_step = ScaleByZoom(1, _settings_client.gui.zoom_min);
	const double max_step = ScaleByZoom(1, _settings_client.gui.zoom_max);
	const double start_step = vp.zoom_step > 0 ? vp.zoom_step : ScaleByZoom(1, vp.zoom);
	const auto existing = _viewport_flights.find(w);
	/* A replacement destination should retain the intended landing zoom,
	 * rather than making the temporary wide travel view the new arrival zoom. */
	const double retained_step = existing != _viewport_flights.end() ? existing->second.path.GetDestination().step : start_step;
	const double end_step = std::clamp(arrival_scale > 0 ? ZOOM_BASE / arrival_scale : retained_step, min_step, max_step);
	if (existing != _viewport_flights.end()) {
		const auto &destination = existing->second.path.GetDestination();
		if (destination.x == centre.x && destination.y == centre.y && existing->second.target == target &&
				(arrival_scale == 0 || std::abs(destination.step - end_step) < 0.000001)) return true;
	}
	const double start_x = vp.scrollpos_x + vp.virtual_width / 2;
	const double start_y = vp.scrollpos_y + vp.virtual_height / 2;
	if (std::abs(centre.x - start_x) < 1 && std::abs(centre.y - start_y) < 1 &&
			std::abs(start_step - end_step) < 0.000001) return false;
	CancelViewportSmoothZoomAnimation(w);
	vp.CancelFollow(*w);
	vp.dest_scrollpos_x = vp.scrollpos_x;
	vp.dest_scrollpos_y = vp.scrollpos_y;
	ViewportFlight path({start_x, start_y, start_step}, {static_cast<double>(centre.x), static_cast<double>(centre.y), end_step},
			min_step, max_step, vp.width, _settings_client.gui.fly_pan_speed,
			_settings_client.gui.fly_duration_scale, _settings_client.gui.fly_zoom_out, _settings_client.gui.fly_curve / 100.0);
	_viewport_flights.insert_or_assign(w, CameraFlight{path, target});
	Debug(Facility::Misc, Severity::Debug1, "Camera flight start: {},{} to {},{}, {:.4f}x arrival, {:.0f} ms planned, target={}",
		start_x, start_y, centre.x, centre.y, ZOOM_BASE / end_step, path.GetDuration(), static_cast<int>(target));
	w->SetDirty();
	if (!path.IsActive()) UpdateViewportFlight(w, 0);
	return true;
}

/**
 * Advance an active navigation flight and its moving destination.
 * @param w Window owning the flight.
 * @param delta_ms Elapsed real time in milliseconds.
 */
static void UpdateViewportFlight(Window *w, uint delta_ms)
{
	auto it = _viewport_flights.find(w);
	if (it == _viewport_flights.end()) return;
	if (!CanStartViewportFlight(w, it->second.target) || w->viewport->follow_vehicle != VehicleID::Invalid()) {
		CancelViewportFlight(w);
		return;
	}
	ViewportData &vp = *w->viewport;
	CameraFlight &flight = it->second;
	flight.elapsed_ms += delta_ms;
	flight.frames++;
	flight.longest_frame_ms = std::max(flight.longest_frame_ms, delta_ms);
	flight.path.Advance(delta_ms, std::max(vp.width, vp.height));
	auto pose = flight.path.GetPose();
	if (flight.vehicle != VehicleID::Invalid()) {
		const Vehicle *vehicle = Vehicle::GetIfValid(flight.vehicle);
		if (vehicle == nullptr) {
			CancelViewportFlight(w);
			return;
		}
		const Vehicle *front = vehicle->GetMovingFront();
		const Point live = RemapCoords(front->x_pos, front->y_pos, front->z_pos);
		pose = flight.path.GetPoseTowards(live.x, live.y);
	}
	const bool zoom_changed = SetViewportSmoothZoom(w, static_cast<float>(ZOOM_BASE / pose.step), false);
	if (!zoom_changed && vp.zoom_step == 0) {
		flight.native_pan_frames++;
		flight.native_pan_ms += delta_ms;
		flight.longest_native_pan_ms = std::max(flight.longest_native_pan_ms, delta_ms);
	}
	vp.scrollpos_x = vp.dest_scrollpos_x = static_cast<int>(std::lround(pose.x)) - vp.virtual_width / 2;
	vp.scrollpos_y = vp.dest_scrollpos_y = static_cast<int>(std::lround(pose.y)) - vp.virtual_height / 2;
	if (vp.overlay != nullptr) vp.overlay->SetDirty();
	if (zoom_changed || vp.overlay != nullptr) {
		/* Scale changes need a complete redraw. Constant-scale translation,
		 * including fractional views, keeps its sampling phase and reuses pixels. */
		vp.virtual_left = vp.scrollpos_x;
		vp.virtual_top = vp.scrollpos_y;
		vp.pixel_phase.Reset(vp.virtual_left, vp.virtual_top, vp.zoom_step);
		w->SetDirty();
	}
	if (zoom_changed) w->InvalidateData();
	if (!flight.path.IsActive()) {
		Debug(Facility::Misc, Severity::Debug1,
				"Camera flight: {} ms elapsed, arrival {:.4f}x, {} frames, {} ms maximum interval; native pan {} frames, {:.1f} ms average, {} ms maximum",
				flight.elapsed_ms, ZOOM_BASE / pose.step, flight.frames, flight.longest_frame_ms,
				flight.native_pan_frames, flight.native_pan_frames == 0 ? 0.0 : static_cast<double>(flight.native_pan_ms) / flight.native_pan_frames,
				flight.longest_native_pan_ms);
		CancelViewportFlight(w);
	}
}


/**
 * Advance camera animation on real time, including while paused.
 * @param w Window owning the camera animation.
 * @param delta_ms Elapsed real time in milliseconds.
 */
void UpdateViewportSmoothZoomAnimation(Window *w, uint delta_ms)
{
	if (delta_ms == 0) return;
	UpdateViewportFlight(w, delta_ms);
	auto it = _smooth_zoom_animations.find(w);
	if (it == _smooth_zoom_animations.end()) return;
	if (w->viewport == nullptr || !_settings_client.gui.smooth_zoom || !BlitterFactory::GetCurrentBlitter()->SupportsSmoothZoom()) {
		CancelViewportSmoothZoomAnimation(w);
		return;
	}

	ViewportData &vp = *w->viewport;
	SmoothZoomAnimation &animation = it->second;
	++animation.frames;
	animation.total_ms += delta_ms;
	animation.longest_frame_ms = std::max(animation.longest_frame_ms, delta_ms);
	const double screen_extent = std::max(vp.width, vp.height);
	animation.motion.Advance(delta_ms, screen_extent);
	const double step = animation.motion.GetStep();
	const int pending_scroll_x = vp.dest_scrollpos_x - vp.scrollpos_x;
	const int pending_scroll_y = vp.dest_scrollpos_y - vp.scrollpos_y;
	/* Accommodate dragging or following while a transition is running. */
	animation.origin_x += vp.scrollpos_x - animation.last_scroll_x;
	animation.origin_y += vp.scrollpos_y - animation.last_scroll_y;
	SetViewportSmoothZoom(w, static_cast<float>(ZOOM_BASE / step), false);
	if (animation.to_cursor) {
		const double actual_step = vp.zoom_step > 0 ? vp.zoom_step : ScaleByZoom(1, vp.zoom);
		vp.scrollpos_x = vp.dest_scrollpos_x = animation.origin_x + GetZoomAnchorOffset(animation.anchor_x, animation.start, actual_step);
		vp.scrollpos_y = vp.dest_scrollpos_y = animation.origin_y + GetZoomAnchorOffset(animation.anchor_y, animation.start, actual_step);
		vp.virtual_left = vp.scrollpos_x;
		vp.virtual_top = vp.scrollpos_y;
		vp.pixel_phase.Reset(vp.virtual_left, vp.virtual_top, vp.zoom_step);
	}
	/* Keep independently requested arrow/edge scrolling or a smooth camera move. */
	vp.dest_scrollpos_x = vp.scrollpos_x + pending_scroll_x;
	vp.dest_scrollpos_y = vp.scrollpos_y + pending_scroll_y;
	animation.last_scroll_x = vp.scrollpos_x;
	animation.last_scroll_y = vp.scrollpos_y;
	w->InvalidateData();
	w->SetDirty();
	if (!animation.motion.IsActive()) {
		Debug(Facility::Misc, Severity::Debug1, "Smooth zoom: {} frames, {} ms elapsed, {:.1f} ms average interval, {} ms maximum interval",
			animation.frames, animation.total_ms, static_cast<double>(animation.total_ms) / animation.frames, animation.longest_frame_ms);
		CancelViewportSmoothZoomAnimation(w);
	}
}

/**
 * Animate a quarter-octave wheel zoom, optionally keeping the cursor position fixed.
 * @return True if smooth zoom handled the event, including reaching a zoom limit.
 * @param in True to zoom in; false to zoom out.
 * @param w Window owning the viewport.
 * @param to_cursor Whether to preserve the map point beneath the cursor.
 * @param notches Number of quarter-octave wheel notches.
 */
bool TrySmoothZoomWindow(bool in, Window *w, bool to_cursor, uint notches)
{
	assert(w != nullptr);
	if (!_settings_client.gui.smooth_zoom || w->viewport == nullptr || !BlitterFactory::GetCurrentBlitter()->SupportsSmoothZoom()) return false;
	CancelViewportFlight(w);

	ViewportData &vp = *w->viewport;
	const double old_step = vp.zoom_step > 0 ? vp.zoom_step : ScaleByZoom(1, vp.zoom);
	const auto it = _smooth_zoom_animations.find(w);
	const double old_target = it != _smooth_zoom_animations.end() ? it->second.motion.GetTargetStep() : old_step;
	const double new_step = GetSmoothZoomStep(old_target, in, _settings_client.gui.zoom_min, _settings_client.gui.zoom_max, notches);
	if (new_step == old_target) return true;

	const int cursor_x = Clamp(_cursor.pos.x - vp.left, 0, vp.width);
	const int cursor_y = Clamp(_cursor.pos.y - vp.top, 0, vp.height);
	SmoothZoomMotion motion = it != _smooth_zoom_animations.end() ? it->second.motion : SmoothZoomMotion(old_step);
	motion.Rebase(old_step);
	motion.Retarget(new_step);
	SmoothZoomAnimation animation{motion, old_step, cursor_x, cursor_y,
		vp.scrollpos_x, vp.scrollpos_y,
		vp.scrollpos_x, vp.scrollpos_y, to_cursor};
	if (it != _smooth_zoom_animations.end()) {
		animation.frames = it->second.frames;
		animation.total_ms = it->second.total_ms;
		animation.longest_frame_ms = it->second.longest_frame_ms;
	}
	_smooth_zoom_animations.insert_or_assign(w, std::move(animation));
	return true;
}
