/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file main_gui.cpp Handling of the main viewport. */

#include "stdafx.h"
#include "spritecache.h"
#include "window_gui.h"
#include "window_func.h"
#include "viewport_func.h"
#include "command_func.h"
#include "console_gui.h"
#include "progress.h"
#include "transparency_gui.h"
#include "map_func.h"
#include "landscape.h"
#include "sound_func.h"
#include "transparency.h"
#include "zoom_func.h"
#include "gfx_type.h"
#include "blitter/factory.hpp"
#include "core/smooth_zoom_motion.hpp"
#include "core/viewport_flight.hpp"
#include "openttd.h"
#include "vehicle_base.h"
#include "debug.h"
#include "toolbar_gui.h"
#include "statusbar_gui.h"
#include "linkgraph/linkgraph_gui.h"
#include "tilehighlight_func.h"
#include "hotkeys.h"
#include "error.h"
#include "news_gui.h"
#include "misc_cmd.h"
#include "timer/timer.h"
#include "timer/timer_window.h"

#include "saveload/saveload_func.h"

#include "widgets/main_widget.h"

#include "network/network.h"
#include "network/network_func.h"
#include "network/network_gui.h"
#include "network/network_base.h"

#include "table/sprites.h"
#include "table/strings.h"

#include <map>

#include "safeguards.h"

/**
 * This code is shared for the majority of the pushbuttons.
 * Handles e.g. the pressing of a button (to build things), playing of click sound and sets certain parameters
 *
 * @param w Window which called the function
 * @param widget ID of the widget (=button) that called this function
 * @param cursor How should the cursor image change? E.g. cursor with depot image in it
 * @param mode Tile highlighting mode, e.g. drawing a rectangle or a dot on the ground
 * @return true if the button is clicked, false if it's unclicked
 */
bool HandlePlacePushButton(Window *w, WidgetID widget, CursorID cursor, HighLightStyle mode)
{
	if (w->IsWidgetDisabled(widget)) return false;

	SndClickBeep();
	w->SetDirty();

	if (w->IsWidgetLowered(widget)) {
		ResetObjectToPlace();
		return false;
	}

	SetObjectToPlace(cursor, PAL_NONE, mode, w->window_class, w->window_number);
	w->LowerWidget(widget);
	return true;
}


void CcPlaySound_EXPLOSION(Commands, const CommandCost &result, TileIndex tile)
{
	if (result.Succeeded() && _settings_client.sound.confirm) SndPlayTileFx(SND_12_EXPLOSION, tile);
}

/**
 * Zooms a viewport in a window in or out.
 * @param how Zooming direction.
 * @param w   Window owning the viewport.
 * @return Returns \c true if zooming step could be done, \c false if further zooming is not possible.
 * @note No button handling or what so ever is done.
 */
bool DoZoomInOutWindow(ZoomStateChange how, Window *w)
{
	assert(w != nullptr);
	if (how != ZOOM_NONE) CancelViewportSmoothZoomAnimation(w);

	/* Fractional zoom buttons move to the next integer level in the requested direction. */
	if (how != ZOOM_NONE && w->viewport != nullptr && w->viewport->zoom_step > 0) {
		ViewportData &vp = *w->viewport;
		const ZoomLevel target = GetNextIntegerZoom(vp.zoom_step, how == ZOOM_IN, _settings_client.gui.zoom_min, _settings_client.gui.zoom_max);
		const float target_step = static_cast<float>(ScaleByZoom(1, target));
		if (target_step == vp.zoom_step) return false;
		SetViewportSmoothZoom(w, static_cast<float>(ZOOM_BASE) / target_step);
		SetViewportSmoothZoom(w, 0);
		w->InvalidateData();
		return true;
	}

	switch (how) {
		case ZOOM_NONE:
			/* On initialisation of the viewport we don't do anything. */
			break;

		case ZOOM_IN: {
			ViewportData &vp = *w->viewport;
			if (vp.zoom <= _settings_client.gui.zoom_min) return false;
			--vp.zoom;
			vp.virtual_width >>= 1;
			vp.virtual_height >>= 1;

			vp.scrollpos_x += vp.virtual_width >> 1;
			vp.scrollpos_y += vp.virtual_height >> 1;
			vp.dest_scrollpos_x = vp.scrollpos_x;
			vp.dest_scrollpos_y = vp.scrollpos_y;
			break;
		}

		case ZOOM_OUT: {
			ViewportData &vp = *w->viewport;
			if (vp.zoom >= _settings_client.gui.zoom_max) return false;
			++vp.zoom;

			vp.scrollpos_x -= vp.virtual_width >> 1;
			vp.scrollpos_y -= vp.virtual_height >> 1;
			vp.dest_scrollpos_x = vp.scrollpos_x;
			vp.dest_scrollpos_y = vp.scrollpos_y;

			vp.virtual_width <<= 1;
			vp.virtual_height <<= 1;
			break;
		}
	}

	if (w->viewport != nullptr) { // the viewport can be null when how == ZOOM_NONE
		w->viewport->virtual_left = w->viewport->scrollpos_x;
		w->viewport->virtual_top = w->viewport->scrollpos_y;
	}

	/* Update the windows that have zoom-buttons to perhaps disable their buttons */
	w->InvalidateData();
	return true;
}

struct SmoothZoomAnimation {
	SmoothZoomMotion motion;
	double start;
	int anchor_x;
	int anchor_y;
	int origin_x;
	int origin_y;
	int last_scroll_x;
	int last_scroll_y;
	bool to_cursor;
	uint frames = 0;
	uint total_ms = 0;
	uint longest_frame_ms = 0;
};

static std::map<Window *, SmoothZoomAnimation> _smooth_zoom_animations;

struct CameraFlight {
	ViewportFlight path;
	ViewportFlightTarget target;
	VehicleID vehicle = VehicleID::Invalid();
	uint elapsed_ms = 0;
	uint frames = 0;
	uint longest_frame_ms = 0;
	uint native_pan_frames = 0;
	uint native_pan_ms = 0;
	uint longest_native_pan_ms = 0;
};

static std::map<Window *, CameraFlight> _viewport_flights;

void CancelViewportFlight(Window *w)
{
	if (_viewport_flights.erase(w) == 0) return;
	if (w != nullptr && w->viewport != nullptr) {
		w->viewport->zoom_animation = false;
		w->viewport->dest_scrollpos_x = w->viewport->scrollpos_x;
		w->viewport->dest_scrollpos_y = w->viewport->scrollpos_y;
		w->SetDirty();
	}
}

void CancelViewportSmoothZoomAnimation(Window *w)
{
	CancelViewportFlight(w);
	if (w != nullptr && w->viewport != nullptr && w->viewport->zoom_animation) {
		w->viewport->zoom_animation = false;
		w->SetDirty(); // Restore settled filtering even if the scale itself is unchanged.
	}
	_smooth_zoom_animations.erase(w);
}

static void UpdateViewportFlight(Window *w, uint delta_ms);

/** Attach a moving vehicle endpoint without changing the navigation category. */
void SetViewportFlightVehicleTarget(Window *w, VehicleID vehicle)
{
	auto it = _viewport_flights.find(w);
	if (it == _viewport_flights.end()) return;
	const Vehicle *selected = Vehicle::GetIfValid(vehicle);
	if (selected == nullptr) return;
	const Vehicle *front = selected->GetMovingFront();
	const Viewport &vp = *w->viewport;
	const auto [width, height] = GetNavigationVehicleExtent(front, ZOOM_BASE, INVALID_COORD);
	const double step = GetNavigationArrivalStep(width, height, vp.width, vp.height, ZOOM_BASE / 2.0,
			ScaleByZoom(1, _settings_client.gui.zoom_min), ScaleByZoom(1, _settings_client.gui.zoom_max));
	const auto destination = it->second.path.GetDestination();
	const auto target = it->second.target;
	/* The exact ID disambiguates overlapping vehicles and trains in shared depots.
	 * This runs in the initiating click, before any flight frame is advanced. */
	if (!TryStartViewportFlight(w, {static_cast<int>(destination.x), static_cast<int>(destination.y)}, ZOOM_BASE / step, target)) {
		CancelViewportFlight(w);
		return;
	}
	it = _viewport_flights.find(w);
	if (it != _viewport_flights.end()) it->second.vehicle = vehicle;
}

/** Start an opt-in main-viewport flight; zero arrival scale preserves the current zoom. */
bool TryStartViewportFlight(Window *w, Point centre, double arrival_scale, ViewportFlightTarget target)
{
	if (!_settings_client.gui.smooth_scroll || !IsViewportFlightEnabled(_settings_client.gui, target) || w == nullptr || w != GetMainWindow() || w->viewport == nullptr ||
			_game_mode == GameMode::Menu || !BlitterFactory::GetCurrentBlitter()->SupportsSmoothZoom()) return false;
	ViewportData &vp = *w->viewport;
	if (vp.width <= 0 || vp.height <= 0) return false;
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
	vp.zoom_animation = true;
	w->SetDirty();
	if (!path.IsActive()) UpdateViewportFlight(w, 0);
	return true;
}

static void UpdateViewportFlight(Window *w, uint delta_ms)
{
	auto it = _viewport_flights.find(w);
	if (it == _viewport_flights.end()) return;
	if (w->viewport == nullptr || !_settings_client.gui.smooth_scroll || !IsViewportFlightEnabled(_settings_client.gui, it->second.target) ||
			!BlitterFactory::GetCurrentBlitter()->SupportsSmoothZoom() || w->viewport->follow_vehicle != VehicleID::Invalid()) {
		CancelViewportFlight(w);
		return;
	}
	ViewportData &vp = *w->viewport;
	CameraFlight &flight = it->second;
	flight.elapsed_ms += delta_ms;
	flight.frames++;
	flight.longest_frame_ms = std::max(flight.longest_frame_ms, delta_ms);
	flight.path.Advance(delta_ms);
	auto pose = flight.path.GetPose();
	if (flight.vehicle != VehicleID::Invalid()) {
		const Vehicle *vehicle = Vehicle::GetIfValid(flight.vehicle);
		if (vehicle == nullptr) { CancelViewportFlight(w); return; }
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
	vp.zoom_animation = flight.path.UseFastSampling(std::max(vp.width, vp.height));
	if (vp.overlay != nullptr) vp.overlay->SetDirty();
	if (zoom_changed || vp.zoom_step > 0 || vp.overlay != nullptr) {
		/* Fractional movement changes sampling phase and needs a complete redraw.
		 * At a constant native zoom, leave the displayed origin for the normal
		 * viewport update to scroll existing pixels and redraw exposed strips. */
		vp.virtual_left = vp.scrollpos_x;
		vp.virtual_top = vp.scrollpos_y;
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

/** Advance on real time, including while the simulation is paused. */
/** Moving follow cameras can use fast sampling; stopped and paused views retain settled quality. */
static bool UseFastPanSampling(const Window *w)
{
	if (w->viewport == nullptr || w->viewport->zoom_step == 0) return false;
	const ViewportData &vp = *w->viewport;
	if (_scrolling_viewport && w == GetMainWindow()) return true;
	/* Fractional panning redraws the entire viewport. Restore settled quality
	 * within the visible easing tail, or on the update after dragging stops. */
	if (vp.follow_vehicle == VehicleID::Invalid() && std::max(std::abs(vp.dest_scrollpos_x - vp.scrollpos_x),
			std::abs(vp.dest_scrollpos_y - vp.scrollpos_y)) > 8 * vp.zoom_step) return true;
	if (_pause_mode.Any()) return false;
	const Vehicle *followed = Vehicle::GetIfValid(w->viewport->follow_vehicle);
	return followed != nullptr && followed->cur_speed != 0;
}

/** Advance camera filtering and animation on real time, including while paused. */
void UpdateViewportSmoothZoomAnimation(Window *w, uint delta_ms)
{
	if (delta_ms == 0) return;
	UpdateViewportFlight(w, delta_ms);
	auto it = _smooth_zoom_animations.find(w);
	if (it == _smooth_zoom_animations.end()) {
		if (w->viewport != nullptr && !_viewport_flights.contains(w)) {
			/* Panning must not change sampling after the camera stops. Keep its
			 * filter until a zoom/flight explicitly chooses another one. This also
			 * avoids a delayed quality redraw when a follow camera stops or pauses. */
			const bool fast = UseFastPanSampling(w) ||
					(w->viewport->zoom_step > 0 && w->viewport->zoom_animation);
			if (w->viewport->zoom_animation != fast) {
				w->viewport->zoom_animation = fast;
				/* Redraw when the sampling mode changes, including a return to native zoom. */
				w->SetDirty();
			}
		}
		return;
	}
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
	/* Change filtering before apparent rest, rather than after the invisible
	 * easing tail. Keep the quality filter for the remaining final frames. */
	vp.zoom_animation = animation.motion.UseFastSampling(screen_extent) || UseFastPanSampling(w);
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
		/* A moving follow camera has not come to rest when wheel easing ends.
		 * Avoid a single quality-filtered frame followed by fast sampling again. */
		vp.zoom_animation = UseFastPanSampling(w);
	}
}

/**
 * Animate a quarter-octave wheel zoom, optionally keeping the cursor position fixed.
 * @return True if smooth zoom handled the event, including reaching a zoom limit.
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
	vp.zoom_animation = true;
	return true;
}

void ZoomInOrOutToCursorWindow(bool in, Window *w)
{
	assert(w != nullptr);

	if (_game_mode != GameMode::Menu) {
		if (TrySmoothZoomWindow(in, w, true)) return;
		if ((in && w->viewport->zoom <= _settings_client.gui.zoom_min) || (!in && w->viewport->zoom >= _settings_client.gui.zoom_max)) return;

		Point pt = GetTileZoomCenterWindow(in, w);
		if (pt.x != -1) {
			ScrollWindowTo(pt.x, pt.y, -1, w, true);

			DoZoomInOutWindow(in ? ZOOM_IN : ZOOM_OUT, w);
		}
	}
}

void FixTitleGameZoom(int zoom_adjust)
{
	if (_game_mode != GameMode::Menu) return;
	CancelViewportSmoothZoomAnimation(GetMainWindow());

	Viewport &vp = *GetMainWindow()->viewport;

	/* Adjust the zoom in/out.
	 * Can't simply add, since operator+ is not defined on the ZoomLevel type. */
	vp.zoom_step = 0;
	vp.zoom = _gui_zoom;
	while (zoom_adjust < 0 && vp.zoom != _settings_client.gui.zoom_min) {
		vp.zoom--;
		zoom_adjust++;
	}
	while (zoom_adjust > 0 && vp.zoom != _settings_client.gui.zoom_max) {
		vp.zoom++;
		zoom_adjust--;
	}

	vp.virtual_width = ScaleByZoom(vp.width, vp.zoom);
	vp.virtual_height = ScaleByZoom(vp.height, vp.zoom);
}

static constexpr std::initializer_list<NWidgetPart> _nested_main_window_widgets = {
	NWidget(NWID_VIEWPORT, Colours::Invalid, WID_M_VIEWPORT), SetResize(1, 1),
};

enum GlobalHotKeys : int32_t {
	GHK_QUIT,
	GHK_ABANDON,
	GHK_CONSOLE,
	GHK_BOUNDING_BOXES,
	GHK_DIRTY_BLOCKS,
	GHK_WIDGET_OUTLINES,
	GHK_CENTER,
	GHK_CENTER_ZOOM,
	GHK_RESET_OBJECT_TO_PLACE,
	GHK_DELETE_WINDOWS,
	GHK_DELETE_NONVITAL_WINDOWS,
	GHK_DELETE_ALL_MESSAGES,
	GHK_REFRESH_SCREEN,
	GHK_CRASH,
	GHK_MONEY,
	GHK_UPDATE_COORDS,
	GHK_TOGGLE_TRANSPARENCY,
	GHK_TOGGLE_INVISIBILITY = GHK_TOGGLE_TRANSPARENCY + 9,
	GHK_TRANSPARENCY_TOOLBAR = GHK_TOGGLE_INVISIBILITY + 8,
	GHK_TRANSPARENCY,
	GHK_CHAT,
	GHK_CHAT_ALL,
	GHK_CHAT_COMPANY,
	GHK_CHAT_SERVER,
	GHK_CLOSE_NEWS,
	GHK_CLOSE_ERROR,
};

struct MainWindow : Window
{
	MainWindow(WindowDesc &desc) : Window(desc)
	{
		this->InitNested(0);
		this->flags.Reset(WindowFlag::WhiteBorder);
		ResizeWindow(this, _screen.width, _screen.height);

		NWidgetViewport *nvp = this->GetWidget<NWidgetViewport>(WID_M_VIEWPORT);
		nvp->InitializeViewport(this, TileXY(32, 32), ScaleZoomGUI(ZoomLevel::Viewport));

		this->viewport->overlay = std::make_shared<LinkGraphOverlay>(this, WID_M_VIEWPORT, CargoTypes{}, CompanyMask{}, 2);
		this->refresh_timeout.Reset();
	}

	/** Refresh the link-graph overlay. */
	void RefreshLinkGraph()
	{
		if (this->viewport->overlay->GetCargoMask().None() ||
				this->viewport->overlay->GetCompanyMask().None()) {
			return;
		}

		this->viewport->overlay->SetDirty();
		this->GetWidget<NWidgetBase>(WID_M_VIEWPORT)->SetDirty(this);
	}

	/** Refresh the link-graph overlay on a regular interval. */
	const IntervalTimer<TimerWindow> refresh_interval = {std::chrono::milliseconds(7650), [this](auto) {
		RefreshLinkGraph();
	}};

	/**
	 * Sometimes when something happened, force an update to the link-graph a bit sooner.
	 *
	 * We don't do it instantly on those changes, as for example when you are scrolling,
	 * constantly refreshing the link-graph would be very slow. So we delay it a bit,
	 * and only draw it once the scrolling settles down.
	 */
	TimeoutTimer<TimerWindow> refresh_timeout = {std::chrono::milliseconds(450), [this]() {
		RefreshLinkGraph();
	}};

	void OnPaint() override
	{
		this->DrawWidgets();
		if (_game_mode == GameMode::Menu) {
			static const std::initializer_list<SpriteID> title_sprites = {SPR_OTTD_O, SPR_OTTD_P, SPR_OTTD_E, SPR_OTTD_N, SPR_OTTD_T, SPR_OTTD_T, SPR_OTTD_D};
			uint letter_spacing = ScaleGUITrad(10);
			int name_width = static_cast<int>(std::size(title_sprites) - 1) * letter_spacing;

			for (const SpriteID &sprite : title_sprites) {
				name_width += GetSpriteSize(sprite).width;
			}
			int off_x = (this->width - name_width) / 2;

			for (const SpriteID &sprite : title_sprites) {
				DrawSprite(sprite, PAL_NONE, off_x, ScaleGUITrad(50));
				off_x += GetSpriteSize(sprite).width + letter_spacing;
			}

			int text_y = this->height - GetCharacterHeight(FontSize::Normal) * 2;
			DrawString(0, this->width - 1, text_y, STR_INTRO_VERSION, TextColour::White, {AlignmentH::Centre, AlignmentV::Middle});
		}
	}

	EventState OnHotkey(int hotkey) override
	{
		if (hotkey == GHK_QUIT) {
			HandleExitGameRequest();
			return EventState::Handled;
		}

		/* Disable all key shortcuts, except quit shortcuts when
		 * generating the world, otherwise they create threading
		 * problem during the generating, resulting in random
		 * assertions that are hard to trigger and debug */
		if (HasModalProgress()) return EventState::NotHandled;

		switch (hotkey) {
			case GHK_ABANDON:
				/* No point returning from the main menu to itself */
				if (_game_mode == GameMode::Menu) return EventState::Handled;
				if (_settings_client.gui.autosave_on_exit) {
					DoExitSave();
					_switch_mode = SwitchMode::Menu;
				} else {
					AskExitToGameMenu();
				}
				return EventState::Handled;

			case GHK_CONSOLE:
				IConsoleSwitch();
				return EventState::Handled;

			case GHK_BOUNDING_BOXES:
				ToggleBoundingBoxes();
				return EventState::Handled;

			case GHK_DIRTY_BLOCKS:
				ToggleDirtyBlocks();
				return EventState::Handled;

			case GHK_WIDGET_OUTLINES:
				ToggleWidgetOutlines();
				return EventState::Handled;
		}

		if (_game_mode == GameMode::Menu) return EventState::NotHandled;

		switch (hotkey) {
			case GHK_CENTER:
			case GHK_CENTER_ZOOM: {
				Point pt = GetTileBelowCursor();
				if (pt.x != -1) {
					bool instant = (hotkey == GHK_CENTER_ZOOM && (this->viewport->zoom_step > 0 || this->viewport->zoom != _settings_client.gui.zoom_min));
					if (hotkey == GHK_CENTER_ZOOM) MaxZoomInOut(ZOOM_IN, this);
					ScrollMainWindowTo(pt.x, pt.y, -1, instant);
				}
				break;
			}

			case GHK_RESET_OBJECT_TO_PLACE:
				CancelViewportFlight(this);
				ResetObjectToPlace();
				break;
			case GHK_DELETE_WINDOWS: CloseNonVitalWindows(); break;
			case GHK_DELETE_NONVITAL_WINDOWS: CloseAllNonVitalWindows(); break;
			case GHK_DELETE_ALL_MESSAGES: DeleteAllMessages(); break;
			case GHK_REFRESH_SCREEN: MarkWholeScreenDirty(); break;

			case GHK_CRASH: // Crash the game
				*(volatile uint8_t *)nullptr = 0;
				break;

			case GHK_MONEY: // Gimme money
				/* You can only cheat for money in singleplayer mode. */
				if (!_networking) Command<Commands::MoneyCheat>::Post(10000000);
				break;

			case GHK_UPDATE_COORDS: // Update the coordinates of all station signs
				UpdateAllVirtCoords();
				break;

			case GHK_TOGGLE_TRANSPARENCY:
			case GHK_TOGGLE_TRANSPARENCY + 1:
			case GHK_TOGGLE_TRANSPARENCY + 2:
			case GHK_TOGGLE_TRANSPARENCY + 3:
			case GHK_TOGGLE_TRANSPARENCY + 4:
			case GHK_TOGGLE_TRANSPARENCY + 5:
			case GHK_TOGGLE_TRANSPARENCY + 6:
			case GHK_TOGGLE_TRANSPARENCY + 7:
			case GHK_TOGGLE_TRANSPARENCY + 8:
				/* Transparency toggle hot keys */
				ToggleTransparency((TransparencyOption)(hotkey - GHK_TOGGLE_TRANSPARENCY));
				MarkWholeScreenDirty();
				break;

			case GHK_TOGGLE_INVISIBILITY:
			case GHK_TOGGLE_INVISIBILITY + 1:
			case GHK_TOGGLE_INVISIBILITY + 2:
			case GHK_TOGGLE_INVISIBILITY + 3:
			case GHK_TOGGLE_INVISIBILITY + 4:
			case GHK_TOGGLE_INVISIBILITY + 5:
			case GHK_TOGGLE_INVISIBILITY + 6:
			case GHK_TOGGLE_INVISIBILITY + 7:
				/* Invisibility toggle hot keys */
				ToggleInvisibilityWithTransparency((TransparencyOption)(hotkey - GHK_TOGGLE_INVISIBILITY));
				MarkWholeScreenDirty();
				break;

			case GHK_TRANSPARENCY_TOOLBAR:
				ShowTransparencyToolbar();
				break;

			case GHK_TRANSPARENCY:
				ResetRestoreAllTransparency();
				break;

			case GHK_CHAT: // smart chat; send to team if any, otherwise to all
				if (_networking) {
					const NetworkClientInfo *cio = NetworkClientInfo::GetByClientID(_network_own_client_id);
					if (cio == nullptr) break;

					ShowNetworkChatQueryWindow(NetworkClientPreferTeamChat(cio) ? NetworkChatDestinationType::Team : NetworkChatDestinationType::Broadcast, cio->client_playas.base());
				}
				break;

			case GHK_CHAT_ALL: // send text message to all clients
				if (_networking) ShowNetworkChatQueryWindow(NetworkChatDestinationType::Broadcast, 0);
				break;

			case GHK_CHAT_COMPANY: // send text to all team mates
				if (_networking) {
					const NetworkClientInfo *cio = NetworkClientInfo::GetByClientID(_network_own_client_id);
					if (cio == nullptr) break;

					ShowNetworkChatQueryWindow(NetworkChatDestinationType::Team, cio->client_playas.base());
				}
				break;

			case GHK_CHAT_SERVER: // send text to the server
				if (_networking && !_network_server) {
					ShowNetworkChatQueryWindow(NetworkChatDestinationType::Client, to_underlying(ClientID::Server));
				}
				break;

			case GHK_CLOSE_NEWS: // close active news window
				if (!HideActiveNewsMessage()) return EventState::NotHandled;
				break;

			case GHK_CLOSE_ERROR: // close active error window
				if (!HideActiveErrorMessage()) return EventState::NotHandled;
				break;

			default: return EventState::NotHandled;
		}
		return EventState::Handled;
	}

	void OnScroll(Point delta) override
	{
		CancelViewportFlight(this);
		this->viewport->scrollpos_x += ViewportScale(*this->viewport, delta.x);
		this->viewport->scrollpos_y += ViewportScale(*this->viewport, delta.y);
		this->viewport->dest_scrollpos_x = this->viewport->scrollpos_x;
		this->viewport->dest_scrollpos_y = this->viewport->scrollpos_y;
		if (this->viewport->zoom_step > 0) this->viewport->zoom_animation = true;
		this->refresh_timeout.Reset();
	}

	void OnMouseWheel(int wheel, WidgetID widget) override
	{
		if (widget != WID_M_VIEWPORT) return;
		if (_settings_client.gui.scrollwheel_scrolling != ScrollWheelScrolling::Off) {
			/* Even over off-map pixels, a zoom gesture must stop the flight. */
			CancelViewportFlight(this);
			bool in = wheel < 0;
			if (TrySmoothZoomWindow(in, this, this->viewport->follow_vehicle == VehicleID::Invalid(), static_cast<uint>(std::abs(wheel)))) return;

			/* When following, only change zoom - otherwise zoom to the cursor. */
			if (this->viewport->follow_vehicle != VehicleID::Invalid()) {
				if (!TrySmoothZoomWindow(in, this, false)) DoZoomInOutWindow(in ? ZOOM_IN : ZOOM_OUT, this);
			} else {
				ZoomInOrOutToCursorWindow(in, this);
			}
		}
	}

	void OnResize() override
	{
		if (this->viewport != nullptr) {
			NWidgetViewport *nvp = this->GetWidget<NWidgetViewport>(WID_M_VIEWPORT);
			nvp->UpdateViewportCoordinates(this);
			this->refresh_timeout.Reset();
		}
	}

	bool OnTooltip([[maybe_unused]] Point pt, WidgetID widget, TooltipCloseCondition close_cond) override
	{
		if (widget != WID_M_VIEWPORT) return false;
		return this->viewport->overlay->ShowTooltip(pt, close_cond);
	}

	/**
	 * Some data on this window has become invalid.
	 * @param data Information about the changed data.
	 * @param gui_scope Whether the call is done from GUI scope. You may not do everything when not in GUI scope. See #InvalidateWindowData() for details.
	 */
	void OnInvalidateData([[maybe_unused]] int data = 0, [[maybe_unused]] bool gui_scope = true) override
	{
		if (!gui_scope) return;
		/* Forward the message to the appropriate toolbar (ingame or scenario editor) */
		InvalidateWindowData(WindowClass::MainToolbar, 0, data, true);
	}

	static inline HotkeyList hotkeys{"global", {
		Hotkey({'Q' | WKC_CTRL, 'Q' | WKC_META}, "quit", GHK_QUIT),
		Hotkey({'W' | WKC_CTRL, 'W' | WKC_META}, "abandon", GHK_ABANDON),
		Hotkey(WKC_BACKQUOTE, "console", GHK_CONSOLE),
		Hotkey('B' | WKC_CTRL, "bounding_boxes", GHK_BOUNDING_BOXES),
		Hotkey('I' | WKC_CTRL, "dirty_blocks", GHK_DIRTY_BLOCKS),
		Hotkey('O' | WKC_CTRL, "widget_outlines", GHK_WIDGET_OUTLINES),
		Hotkey('C', "center", GHK_CENTER),
		Hotkey('Z', "center_zoom", GHK_CENTER_ZOOM),
		Hotkey(WKC_ESC, "reset_object_to_place", GHK_RESET_OBJECT_TO_PLACE),
		Hotkey(WKC_DELETE, "delete_windows", GHK_DELETE_WINDOWS),
		Hotkey(WKC_DELETE | WKC_SHIFT, "delete_all_windows", GHK_DELETE_NONVITAL_WINDOWS),
		Hotkey(WKC_DELETE | WKC_CTRL, "delete_all_messages", GHK_DELETE_ALL_MESSAGES),
		Hotkey('R' | WKC_CTRL, "refresh_screen", GHK_REFRESH_SCREEN),
#if defined(_DEBUG)
		Hotkey('0' | WKC_ALT, "crash_game", GHK_CRASH),
		Hotkey('1' | WKC_ALT, "money", GHK_MONEY),
		Hotkey('2' | WKC_ALT, "update_coordinates", GHK_UPDATE_COORDS),
#endif
		Hotkey('1' | WKC_CTRL, "transparency_signs", GHK_TOGGLE_TRANSPARENCY),
		Hotkey('2' | WKC_CTRL, "transparency_trees", GHK_TOGGLE_TRANSPARENCY + 1),
		Hotkey('3' | WKC_CTRL, "transparency_houses", GHK_TOGGLE_TRANSPARENCY + 2),
		Hotkey('4' | WKC_CTRL, "transparency_industries", GHK_TOGGLE_TRANSPARENCY + 3),
		Hotkey('5' | WKC_CTRL, "transparency_buildings", GHK_TOGGLE_TRANSPARENCY + 4),
		Hotkey('6' | WKC_CTRL, "transparency_bridges", GHK_TOGGLE_TRANSPARENCY + 5),
		Hotkey('7' | WKC_CTRL, "transparency_structures", GHK_TOGGLE_TRANSPARENCY + 6),
		Hotkey('8' | WKC_CTRL, "transparency_catenary", GHK_TOGGLE_TRANSPARENCY + 7),
		Hotkey('9' | WKC_CTRL, "transparency_loading", GHK_TOGGLE_TRANSPARENCY + 8),
		Hotkey('1' | WKC_CTRL | WKC_SHIFT, "invisibility_signs", GHK_TOGGLE_INVISIBILITY),
		Hotkey('2' | WKC_CTRL | WKC_SHIFT, "invisibility_trees", GHK_TOGGLE_INVISIBILITY + 1),
		Hotkey('3' | WKC_CTRL | WKC_SHIFT, "invisibility_houses", GHK_TOGGLE_INVISIBILITY + 2),
		Hotkey('4' | WKC_CTRL | WKC_SHIFT, "invisibility_industries", GHK_TOGGLE_INVISIBILITY + 3),
		Hotkey('5' | WKC_CTRL | WKC_SHIFT, "invisibility_buildings", GHK_TOGGLE_INVISIBILITY + 4),
		Hotkey('6' | WKC_CTRL | WKC_SHIFT, "invisibility_bridges", GHK_TOGGLE_INVISIBILITY + 5),
		Hotkey('7' | WKC_CTRL | WKC_SHIFT, "invisibility_structures", GHK_TOGGLE_INVISIBILITY + 6),
		Hotkey('8' | WKC_CTRL | WKC_SHIFT, "invisibility_catenary", GHK_TOGGLE_INVISIBILITY + 7),
		Hotkey('X' | WKC_CTRL, "transparency_toolbar", GHK_TRANSPARENCY_TOOLBAR),
		Hotkey('X', "toggle_transparency", GHK_TRANSPARENCY),
		Hotkey({WKC_RETURN, 'T'}, "chat", GHK_CHAT),
		Hotkey({WKC_SHIFT | WKC_RETURN, WKC_SHIFT | 'T'}, "chat_all", GHK_CHAT_ALL),
		Hotkey({WKC_CTRL | WKC_RETURN, WKC_CTRL | 'T'}, "chat_company", GHK_CHAT_COMPANY),
		Hotkey({WKC_CTRL | WKC_SHIFT | WKC_RETURN, WKC_CTRL | WKC_SHIFT | 'T'}, "chat_server", GHK_CHAT_SERVER),
		Hotkey(WKC_SPACE, "close_news", GHK_CLOSE_NEWS),
		Hotkey(WKC_SPACE, "close_error", GHK_CLOSE_ERROR),
	}};
};

/** Window definition for the main window. */
static WindowDesc _main_window_desc(
	WindowPosition::Manual, {}, 0, 0,
	WindowClass::MainWindow, WindowClass::None,
	WindowDefaultFlag::NoClose,
	_nested_main_window_widgets,
	&MainWindow::hotkeys
);

/**
 * Does the given keycode match one of the keycodes bound to 'quit game'?
 * @param keycode The keycode that was pressed by the user.
 * @return True iff the keycode matches one of the hotkeys for 'quit'.
 */
bool IsQuitKey(uint16_t keycode)
{
	int num = MainWindow::hotkeys.CheckMatch(keycode);
	return num == GHK_QUIT;
}


void ShowSelectGameWindow();

/**
 * Initialise the default colours (remaps and the likes), and load the main windows.
 */
void SetupColoursAndInitialWindow()
{
	for (Colours i : EnumRange(Colours::End)) {
		const uint8_t *b = GetNonSprite(GetColourPalette(i), SpriteType::Recolour) + 1;
		assert(b != nullptr);
		for (Shade j : EnumRange(Shade::End)) {
			SetColourGradient(i, j, PixelColour{b[0xC6 + to_underlying(j)]});
		}
	}

	new MainWindow(_main_window_desc);

	/* XXX: these are not done */
	switch (_game_mode) {
		default: NOT_REACHED();
		case GameMode::Menu:
			ShowSelectGameWindow();
			break;

		case GameMode::Normal:
		case GameMode::Editor:
			ShowVitalWindows();
			break;
	}
}

/**
 * Show the vital in-game windows.
 */
void ShowVitalWindows()
{
	AllocateToolbar();

	/* Status bad only for normal games */
	if (_game_mode == GameMode::Editor) return;

	ShowStatusBar();
}

/**
 * Size of the application screen changed.
 * Adapt the game screen-size, re-allocate the open windows, and repaint everything
 */
void GameSizeChanged()
{
	_cur_resolution.width  = _screen.width;
	_cur_resolution.height = _screen.height;
	ScreenSizeChanged();
	RelocateAllWindows(_screen.width, _screen.height);
	MarkWholeScreenDirty();
}
