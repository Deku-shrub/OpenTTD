/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file smooth_zoom_motion.hpp Real-time zoom motion with continuous velocity. */
#ifndef SMOOTH_ZOOM_MOTION_HPP
#define SMOOTH_ZOOM_MOTION_HPP

#include <cmath>

/** Analytically integrated critically damped motion in logarithmic zoom space. */
class SmoothZoomMotion {
	double position; ///< Current base-2 logarithm of the virtual-pixel step.
	double velocity = 0; ///< Logarithmic step velocity per second.
	double target; ///< Target logarithmic step.
	double target_step; ///< Exact requested step, used after settling.
	bool active = false; ///< Whether easing is still in progress.

public:
	/**
	 * Create a settled zoom controller.
	 * @param step Initial virtual pixels per screen pixel.
	 */
	explicit SmoothZoomMotion(double step) : position(std::log2(step)), target(this->position), target_step(step) {}

	/**
	 * Reconcile the displayed float scale without discarding the current velocity.
	 * @param step Step actually displayed after float rounding.
	 */
	void Rebase(double step) { this->position = std::log2(step); }

	/**
	 * Change the requested step while preserving velocity.
	 * @param step New positive virtual-pixel step.
	 */
	void Retarget(double step)
	{
		if (step == this->target_step) return;
		this->target_step = step;
		this->target = std::log2(step);
		this->active = true;
	}

	/**
	 * Exact solution is independent of the division into display frames.
	 * @param delta_ms Elapsed real time in milliseconds.
	 * @param screen_extent Viewport span in screen pixels; zero uses numerical settling.
	 */
	void Advance(unsigned int delta_ms, double screen_extent = 0)
	{
		if (!this->active || delta_ms == 0) return;
		/* Reach about 90% of a notch in 150 ms, leaving visible intermediate
		 * scales even on displays rendering at roughly 30 frames per second. */
		constexpr double frequency = 26;
		const double time = delta_ms / 1000.0;
		const double error = this->position - this->target;
		const double impulse = this->velocity + frequency * error;
		const double decay = std::exp(-frequency * time);
		this->position = this->target + (error + impulse * time) * decay;
		this->velocity = (this->velocity - frequency * impulse * time) * decay;
		/* Finish the subpixel tail in the last visible frame. Extent is the full
		 * viewport span, so this bounds residual scale displacement to one
		 * pixel even for an off-centre anchor. A longer tail lets edges settle later. */
		const bool visually_settled = screen_extent > 0 && this->GetRemainingPixels(screen_extent) <= 1 &&
				this->GetFrameMovementPixels(screen_extent) <= 1;
		if (screen_extent > 0 ? visually_settled :
				(std::abs(this->position - this->target) < 0.0001 && std::abs(this->velocity) < 0.01)) {
			this->position = this->target;
			this->velocity = 0;
			this->active = false;
		}
	}

	/**
	 * Read the current zoom step.
	 * @return Virtual pixels per screen pixel.
	 */
	double GetStep() const { return this->active ? std::exp2(this->position) : this->target_step; }
	/**
	 * Read the requested zoom step.
	 * @return Exact target step.
	 */
	double GetTargetStep() const { return this->target_step; }
	/**
	 * Read the current easing velocity.
	 * @return Logarithmic step change per second.
	 */
	double GetVelocity() const { return this->velocity; }
	/**
	 * Check whether easing remains.
	 * @return True while the transition is active.
	 */
	bool IsActive() const { return this->active; }


private:
	/**
	 * Bound the remaining scale displacement.
	 * @param screen_extent Viewport span in screen pixels.
	 * @return Remaining displacement in screen pixels.
	 */
	double GetRemainingPixels(double screen_extent) const
	{
		return screen_extent * std::abs(std::exp2(this->target - this->position) - 1);
	}

	/**
	 * Estimated displacement over a nominal 30 ms frame, including reversal momentum.
	 * @param screen_extent Viewport span in screen pixels.
	 * @return Estimated screen-pixel movement in 30 milliseconds.
	 */
	double GetFrameMovementPixels(double screen_extent) const
	{
		return screen_extent * std::abs(this->velocity) * std::log(2.0) * 0.03;
	}
};

#endif /* SMOOTH_ZOOM_MOTION_HPP */
