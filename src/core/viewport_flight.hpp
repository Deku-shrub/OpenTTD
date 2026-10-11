/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file viewport_flight.hpp Bounded perceptual zoom-and-pan navigation in virtual coordinates. */
#ifndef VIEWPORT_FLIGHT_HPP
#define VIEWPORT_FLIGHT_HPP

#include <algorithm>
#include <cmath>
#include <utility>

/**
 * Choose a traditional level that fits an estimated destination within 80% of the viewport.
 * @param virtual_width Destination width in virtual pixels.
 * @param virtual_height Destination height in virtual pixels.
 * @param width Viewport width in screen pixels.
 * @param height Viewport height in screen pixels.
 * @param normal_step Closest desired arrival step.
 * @param min_step Minimum permitted virtual-pixel step.
 * @param max_step Maximum permitted virtual-pixel step.
 * @return Traditional virtual-pixel step fitting the destination within the zoom bounds.
 */
inline double GetNavigationArrivalStep(double virtual_width, double virtual_height, double width, double height,
		double normal_step, double min_step, double max_step)
{
	const double required = std::max({normal_step, virtual_width / (width * 0.8), virtual_height / (height * 0.8)});
	return std::clamp(std::exp2(std::ceil(std::log2(required))), min_step, max_step);
}

/**
 * Include every vehicle part, using world positions even if sprite bounds are not cached yet.
 * @tparam VehicleType Vehicle type exposing positions, bounds and consist links.
 * @param front Selected vehicle or consist front.
 * @param zoom_base Virtual pixels per traditional map pixel.
 * @param invalid_coord Sentinel for uncached sprite bounds.
 * @return Destination width and height in virtual pixels.
 */
template <typename VehicleType>
std::pair<double, double> GetNavigationVehicleExtent(const VehicleType *front, double zoom_base, int invalid_coord)
{
	double half_width = 0, half_height = 0;
	const double centre_x = (static_cast<double>(front->y_pos) - front->x_pos) * 2 * zoom_base;
	const double centre_y = (static_cast<double>(front->x_pos) + front->y_pos - front->z_pos) * zoom_base;
	for (const auto *part = front->First(); part != nullptr; part = part->Next()) {
		const double px = (static_cast<double>(part->y_pos) - part->x_pos) * 2 * zoom_base;
		const double py = (static_cast<double>(part->x_pos) + part->y_pos - part->z_pos) * zoom_base;
		half_width = std::max(half_width, std::abs(px - centre_x));
		half_height = std::max(half_height, std::abs(py - centre_y));
		if (part->coord.left == invalid_coord) continue;
		half_width = std::max({half_width, std::abs(part->coord.left - centre_x), std::abs(part->coord.right - centre_x)});
		half_height = std::max({half_height, std::abs(part->coord.top - centre_y), std::abs(part->coord.bottom - centre_y)});
	}
	return {std::max(256 * zoom_base, 2 * half_width + 64 * zoom_base),
			std::max(160 * zoom_base, 2 * half_height + 80 * zoom_base)};
}

/** Finite camera flight, sampled by elapsed real time rather than accumulated frame deltas. */
class ViewportFlight {
public:
	/** Camera centre and scale in virtual coordinates. */
	struct Pose {
		double x; ///< Virtual x coordinate of the camera centre.
		double y; ///< Virtual y coordinate of the camera centre.
		double step; ///< Virtual pixels per screen pixel.
	};

private:
	Pose start; ///< Initial camera pose.
	Pose destination; ///< Requested arrival pose.
	double min_step; ///< Minimum permitted virtual-pixel step.
	double max_step; ///< Maximum permitted virtual-pixel step.
	double peak_step; ///< Maximum step reached by the bounded path.
	double distance; ///< Centre distance normalised by viewport width.
	double rho; ///< Perceptual path curvature.
	double r0 = 0; ///< Initial hyperbolic path coordinate.
	double r_span = 0; ///< Span of hyperbolic path coordinates.
	double path_length = 0; ///< Integrated perceptual path length.
	double duration_ms = 0; ///< Bounded total flight duration in milliseconds.
	double elapsed_ms = 0; ///< Elapsed flight time in milliseconds.
	bool flat = false; ///< Whether the path omits extra zoom-out.

	/**
	 * Apply quintic easing with zero endpoint velocity and acceleration.
	 * @param t Normalised progress, clamped to zero through one.
	 * @return Eased progress.
	 */
	static double Ease(double t)
	{
		t = std::clamp(t, 0.0, 1.0);
		return t * t * t * (10 + t * (-15 + 6 * t));
	}

	/**
	 * Compute the van Wijk-Nuij path in viewport-width units, avoiding cancellation in log(sqrt(b*b+1)-b).
	 * @param curve Positive perceptual curvature parameter.
	 */
	void MakeCurve(double curve)
	{
		this->rho = curve;
		const double w0 = this->start.step, w1 = this->destination.step;
		const double rho2d = curve * curve * this->distance;
		const double difference = (w1 - w0) * (w1 + w0);
		this->r0 = -std::asinh((difference + rho2d * rho2d) / (2 * w0 * rho2d));
		const double r1 = -std::asinh((difference - rho2d * rho2d) / (2 * w1 * rho2d));
		this->r_span = r1 - this->r0;
		this->path_length = this->r_span / curve;
		const double apex = std::clamp(0.0, this->r0, r1);
		this->peak_step = w0 * std::cosh(this->r0) / std::cosh(apex);
	}

public:
	/**
	 * Construct a bounded simultaneous zoom-and-pan path.
	 * @param start Initial camera pose.
	 * @param destination Requested arrival pose.
	 * @param min_step Minimum permitted virtual-pixel step.
	 * @param max_step Maximum permitted virtual-pixel step.
	 * @param screen_extent Viewport width in screen pixels.
	 * @param speed_percent Perceptual navigation speed as a percentage.
	 * @param duration_percent Duration multiplier as a percentage.
	 * @param extra_levels Additional zoom-out allowance in octaves.
	 * @param curve Requested perceptual curvature.
	 */
	ViewportFlight(Pose start, Pose destination, double min_step, double max_step, double screen_extent,
			double speed_percent, double duration_percent, double extra_levels, double curve = 1.42) :
		start(start), destination(destination), min_step(min_step), max_step(max_step), rho(curve)
	{
		this->start.step = std::clamp(start.step, min_step, max_step);
		this->destination.step = std::clamp(destination.step, min_step, max_step);
		this->distance = std::hypot(destination.x - start.x, destination.y - start.y) / std::max(1.0, screen_extent);
		const double w0 = this->start.step, w1 = this->destination.step;
		const double base = std::max(w0, w1);
		const double cap = std::min(max_step, base * std::exp2(std::max(0.0, extra_levels)));
		/* Short hops need no extra pull-back, even when the arrival zoom differs. */
		this->flat = this->distance < w0 || cap <= base || this->distance < 1e-6;
		if (!this->flat) {
			this->MakeCurve(curve);
			if (this->peak_step > cap) {
				/* Lower curvature keeps pan and zoom simultaneous within the zoom-out cap. */
				this->MakeCurve(0.05);
				if (this->peak_step > cap) {
					this->flat = true;
				} else {
					double low = 0.05, high = curve;
					for (int i = 0; i < 40; ++i) {
						const double middle = (low + high) / 2;
						this->MakeCurve(middle);
						if (this->peak_step > cap) high = middle; else low = middle;
					}
					this->MakeCurve(low);
				}
			}
		}
		if (this->flat) {
			this->rho = curve;
			this->peak_step = base;
			/* Integrate the same perceptual metric for a no-pull-back path. Pure zoom/pan are exact. */
			const double zoom = std::log(w1 / w0);
			this->path_length = 0;
			for (int i = 0; i < 64; ++i) {
				const double width = w0 * std::exp(zoom * (i + 0.5) / 64);
				this->path_length += std::hypot(curve * this->distance / width, zoom / curve) / 64;
			}
		}
		const double raw_ms = 1000 * this->path_length / std::max(0.01, speed_percent / 100) * duration_percent / 100;
		/* Accelerate long routes to the time cap so enabled flights do not become instant jumps. */
		this->duration_ms = std::clamp(raw_ms, 300.0, 2200.0);
	}

	/**
	 * Finish the invisible landing tail when combined pan/zoom error is below a pixel.
	 * @param delta_ms Elapsed real time in milliseconds.
	 * @param screen_extent Screen-pixel span for a subpixel landing bound; zero disables it.
	 */
	void Advance(unsigned int delta_ms, double screen_extent = 0)
	{
		this->elapsed_ms = std::min(this->elapsed_ms + delta_ms, this->GetDuration());
		if (screen_extent <= 0 || this->GetProgress() <= 0.5 || !this->IsActive()) return;
		const Pose pose = this->GetPose();
		const double pan_pixels = std::hypot(this->destination.x - pose.x, this->destination.y - pose.y) / pose.step;
		const double zoom_pixels = screen_extent * std::abs(this->destination.step / pose.step - 1);
		if (pan_pixels + zoom_pixels <= 1) this->elapsed_ms = this->GetDuration();
	}
	/**
	 * Read the bounded duration.
	 * @return Flight duration in milliseconds.
	 */
	double GetDuration() const { return this->duration_ms; }
	/**
	 * Read the perceptual path length.
	 * @return Integrated perceptual path length.
	 */
	double GetPathLength() const { return this->path_length; }
	/**
	 * Read elapsed normalised progress.
	 * @return Progress from zero to one.
	 */
	double GetProgress() const { return this->duration_ms == 0 ? 1 : this->elapsed_ms / this->duration_ms; }
	/**
	 * Read the requested arrival pose.
	 * @return Original destination pose.
	 */
	const Pose &GetDestination() const { return this->destination; }
	/**
	 * Check whether a flight remains.
	 * @return True before the duration is reached.
	 */
	bool IsActive() const { return this->elapsed_ms < this->GetDuration(); }

	/**
	 * Sample the path at the current elapsed time.
	 * @return Current camera centre and step.
	 */
	Pose GetPose() const
	{
		if (!this->IsActive()) return this->destination;
		if (this->elapsed_ms == 0) return this->start;
		const double progress = Ease(this->GetProgress());
		double pan = progress, step;
		if (this->flat) {
			step = this->start.step == this->destination.step ? this->start.step :
					std::exp2(std::log2(this->start.step) + progress * std::log2(this->destination.step / this->start.step));
		} else {
			const double r = this->r0 + progress * this->r_span;
			/* Hyperbolic identity avoids subtracting almost equal sinh/cosh terms. */
			pan = this->start.step * std::sinh(progress * this->r_span) /
					(this->rho * this->rho * this->distance * std::cosh(r));
			step = this->start.step * std::cosh(this->r0) / std::cosh(r);
		}
		pan = std::clamp(pan, 0.0, 1.0);
		return {this->start.x + (this->destination.x - this->start.x) * pan,
				this->start.y + (this->destination.y - this->start.y) * pan,
				std::clamp(step, this->min_step, std::min(this->max_step, this->peak_step))};
	}

	/**
	 * Blend a moving destination only near arrival, without restarting the flight clock.
	 * @param x Live destination centre in virtual x coordinates.
	 * @param y Live destination centre in virtual y coordinates.
	 * @return Pose adjusted towards a moving destination near arrival.
	 */
	Pose GetPoseTowards(double x, double y) const
	{
		Pose pose = this->GetPose();
		if (!this->IsActive()) return {x, y, pose.step};
		const double blend = Ease((this->GetProgress() - 0.8) / 0.2);
		pose.x += (x - this->destination.x) * blend;
		pose.y += (y - this->destination.y) * blend;
		return pose;
	}

};

#endif /* VIEWPORT_FLIGHT_HPP */
