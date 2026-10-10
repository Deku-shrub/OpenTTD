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
	double distance;
	double rho;
	double r0 = 0;
	double r_span = 0;
	double path_length = 0;
	double duration_ms = 0;
	double elapsed_ms = 0;
	bool flat = false;

	static double Ease(double t)
	{
		t = std::clamp(t, 0.0, 1.0);
		return t * t * t * (10 + t * (-15 + 6 * t));
	}

	/** Compute the van Wijk-Nuij path in viewport-width units, avoiding cancellation in log(sqrt(b*b+1)-b). */
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
		/* Short hops keep the chosen zoom, or interpolate an explicit arrival zoom without a bob. */
		this->flat = this->distance < w0 || cap <= base || this->distance < 1e-6;
		if (!this->flat) {
			this->MakeCurve(curve);
			if (this->peak_step > cap) {
				/* Flatten the simultaneous curve rather than introducing three separate stages. */
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
		/* Keep enabled navigation animated, accelerating long routes to the time cap.
		 * An automatic cut after a town arrival looks like the feature stopped working. */
		this->duration_ms = std::clamp(raw_ms, 300.0, 2200.0);
	}

	void Advance(unsigned int delta_ms) { this->elapsed_ms = std::min(this->elapsed_ms + delta_ms, this->GetDuration()); }
	double GetDuration() const { return this->duration_ms; }
	double GetPathLength() const { return this->path_length; }
	double GetProgress() const { return this->duration_ms == 0 ? 1 : this->elapsed_ms / this->duration_ms; }
	const Pose &GetDestination() const { return this->destination; }
	bool IsActive() const { return this->elapsed_ms < this->GetDuration(); }

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

	/** Blend a moving destination only near arrival, without restarting the flight clock. */
	Pose GetPoseTowards(double x, double y) const
	{
		Pose pose = this->GetPose();
		if (!this->IsActive()) return {x, y, pose.step};
		const double blend = Ease((this->GetProgress() - 0.8) / 0.2);
		pose.x += (x - this->destination.x) * blend;
		pose.y += (y - this->destination.y) * blend;
		return pose;
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
