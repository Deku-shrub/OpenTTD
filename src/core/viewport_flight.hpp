/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file viewport_flight.hpp Bounded parabolic camera navigation in virtual coordinates. */
#ifndef VIEWPORT_FLIGHT_HPP
#define VIEWPORT_FLIGHT_HPP

#include <algorithm>
#include <cmath>

/** Choose a traditional level that fits an estimated destination within 80% of the viewport. */
inline double GetNavigationArrivalStep(double virtual_width, double virtual_height, double width, double height,
		double normal_step, double min_step, double max_step)
{
	const double required = std::max({normal_step, virtual_width / (width * 0.8), virtual_height / (height * 0.8)});
	return std::clamp(std::exp2(std::ceil(std::log2(required))), min_step, max_step);
}

/** Finite camera flight, sampled by elapsed real time rather than accumulated frame deltas. */
class ViewportFlight {
public:
	struct Pose { double x, y, step; };

private:
	Pose start;
	Pose destination;
	double min_step;
	double max_step;
	double peak_step;
	double zoom_out_ms;
	double zoom_in_ms;
	double pan_ms;
	double elapsed_ms = 0;

	static double Ease(double t)
	{
		t = std::clamp(t, 0.0, 1.0);
		return t * t * t * (10 + t * (-15 + 6 * t));
	}

public:
	ViewportFlight(Pose start, Pose destination, double min_step, double max_step, double screen_extent,
			double pan_speed_percent, double zoom_ms, double extra_levels) :
		start(start), destination(destination), min_step(min_step), max_step(max_step)
	{
		this->start.step = std::clamp(start.step, min_step, max_step);
		this->destination.step = std::clamp(destination.step, min_step, max_step);
		const double distance = std::hypot(destination.x - start.x, destination.y - start.y);
		const double base_step = std::max(this->start.step, this->destination.step);
		/* Short trips should not pull the camera needlessly away from its subject.
		 * Longer trips reveal more of the route, within the configured zoom limits. */
		const double route_step = std::max(base_step, distance / (screen_extent * 0.7));
		const double route_zoom = route_step > base_step * 1.25 ? route_step : base_step;
		this->peak_step = std::clamp(route_zoom, base_step, std::min(max_step, base_step * std::exp2(extra_levels)));
		this->zoom_out_ms = this->peak_step == this->start.step ? 0 : zoom_ms;
		this->zoom_in_ms = this->peak_step == this->destination.step ? 0 : zoom_ms;
		this->pan_ms = std::clamp(1000 * distance / (screen_extent * this->peak_step * pan_speed_percent / 100), 150.0, 4000.0);
	}

	void Advance(unsigned int delta_ms) { this->elapsed_ms = std::min(this->elapsed_ms + delta_ms, this->GetDuration()); }
	double GetDuration() const { return this->zoom_out_ms + this->pan_ms + this->zoom_in_ms; }
	const Pose &GetDestination() const { return this->destination; }
	bool IsActive() const { return this->elapsed_ms < this->GetDuration(); }

	Pose GetPose() const
	{
		if (!this->IsActive()) return this->destination;
		const double pan = Ease(this->elapsed_ms / this->GetDuration());
		/* A quadratic Bezier in logarithmic zoom space shares pan progress.
		 * This control point makes its maximum exactly the bounded peak, also
		 * for unequal endpoint zooms, without a stationary travel plateau. */
		const double start_zoom = std::log2(this->start.step);
		const double arrival_zoom = std::log2(this->destination.step);
		const double peak_zoom = std::log2(this->peak_step);
		const double control_zoom = peak_zoom + std::sqrt(std::max(0.0,
				(peak_zoom - start_zoom) * (peak_zoom - arrival_zoom)));
		/* Polynomial form preserves an exactly constant zoom without round-off
		 * from summing Bezier weights. Clamp the peak against floating error. */
		const double step = std::exp2(start_zoom + pan * (2 * (control_zoom - start_zoom) +
				pan * (start_zoom - 2 * control_zoom + arrival_zoom)));
		return {this->start.x + (this->destination.x - this->start.x) * pan,
				this->start.y + (this->destination.y - this->start.y) * pan,
				std::clamp(step, this->min_step, this->peak_step)};
	}

	/** Restore filtering during the last visible movement, avoiding delayed sharpening. */
	bool UseFastSampling(double screen_extent) const
	{
		if (!this->IsActive()) return false;
		/* Keep the first half inexpensive; restore filtering before arrival. */
		if (this->elapsed_ms < this->GetDuration() * 0.5) return true;
		const Pose pose = this->GetPose();
		const double pan_pixels = std::hypot(this->destination.x - pose.x, this->destination.y - pose.y) / pose.step;
		const double zoom_pixels = screen_extent * std::abs(this->destination.step / pose.step - 1);
		return pan_pixels > 8 || zoom_pixels > 8;
	}
};

#endif /* VIEWPORT_FLIGHT_HPP */
