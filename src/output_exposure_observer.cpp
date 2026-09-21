#include "output_exposure_observer.hpp"

#include <algorithm>
#include <cmath>

namespace HYP36ROutputExposure
{
	namespace
	{
		Observer observer;

		float finite_or_zero(float value)
		{
			return std::isfinite(value) ? value : 0.0f;
		}
	}

	const Frame& Observer::observe(float rawComposerInput, float rawPostTanh,
		float rawPreDriveOutput, float deltaSeconds)
	{
		const float composerInput = finite_or_zero(rawComposerInput);
		const float postTanh = finite_or_zero(rawPostTanh);
		const float preDriveOutput = finite_or_zero(rawPreDriveOutput);
		const float magnitude = std::abs(preDriveOutput);

		current_.composerInput = composerInput;
		current_.postTanh = postTanh;
		current_.instantaneousMagnitude = magnitude;
		current_.normalizedHeadroom = (std::clamp)(1.0f - magnitude, 0.0f, 1.0f);
		current_.preTanhOverUnity = std::abs(composerInput) > 1.0f;
		current_.nearBoundary = magnitude >= 0.98f;
		current_.directInputClampActive = false;

		if (havePrevious_ && std::isfinite(deltaSeconds) &&
			deltaSeconds > 0.0f && deltaSeconds <= 0.5f)
		{
			current_.outputSlewPerSecond = finite_or_zero(
				std::abs(preDriveOutput - previousPreDrive_) / deltaSeconds);
		}
		else
		{
			current_.outputSlewPerSecond = 0.0f;
		}
		previousPreDrive_ = preDriveOutput;
		havePrevious_ = true;

		magnitudes_[nextSample_] = magnitude;
		nextSample_ = (nextSample_ + 1) % magnitudes_.size();
		sampleCount_ = (std::min)(sampleCount_ + 1, magnitudes_.size());

		auto recent_value = [&](size_t age)
		{
			const size_t index = (nextSample_ + magnitudes_.size() - 1 - age) % magnitudes_.size();
			return magnitudes_[index];
		};
		auto rms = [&](size_t requestedSamples)
		{
			const size_t count = (std::min)(sampleCount_, requestedSamples);
			float squaredSum = 0.0f;
			for (size_t age = 0; age < count; ++age)
			{
				const float value = recent_value(age);
				squaredSum += value * value;
			}
			return count != 0 ? std::sqrt(squaredSum / static_cast<float>(count)) : 0.0f;
		};

		const size_t peakCount = (std::min)(sampleCount_, RecentPeakSamples);
		current_.recentPeak = 0.0f;
		for (size_t age = 0; age < peakCount; ++age)
			current_.recentPeak = (std::max)(current_.recentPeak, recent_value(age));

		current_.sustainedOneSecond = rms(SustainedOneSecondSamples);
		current_.sustainedThreeSeconds = rms(SustainedThreeSecondSamples);

		float counts[4]{};
		for (size_t age = 0; age < sampleCount_; ++age)
		{
			const float value = recent_value(age);
			counts[0] += value >= 0.50f ? 1.0f : 0.0f;
			counts[1] += value >= 0.75f ? 1.0f : 0.0f;
			counts[2] += value >= 0.90f ? 1.0f : 0.0f;
			counts[3] += value >= 0.98f ? 1.0f : 0.0f;
		}
		const float denominator = sampleCount_ != 0 ? static_cast<float>(sampleCount_) : 1.0f;
		current_.occupancy50 = counts[0] / denominator;
		current_.occupancy75 = counts[1] / denominator;
		current_.occupancy90 = counts[2] / denominator;
		current_.occupancy98 = counts[3] / denominator;

		return current_;
	}

	void Observer::observe_directinput_clamp(bool active)
	{
		current_.directInputClampActive = active;
	}

	void Observer::reset()
	{
		magnitudes_.fill(0.0f);
		nextSample_ = 0;
		sampleCount_ = 0;
		previousPreDrive_ = 0.0f;
		havePrevious_ = false;
		current_ = {};
		current_.normalizedHeadroom = 1.0f;
	}

	const Frame& observe(float composerInput, float postTanh, float preDriveOutput,
		float deltaSeconds)
	{
		return observer.observe(composerInput, postTanh, preDriveOutput, deltaSeconds);
	}

	void observe_directinput_clamp(bool active) { observer.observe_directinput_clamp(active); }
	void reset() { observer.reset(); }
	const Frame& frame() { return observer.frame(); }
}
