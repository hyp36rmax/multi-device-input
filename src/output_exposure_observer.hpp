#pragma once

#include <array>
#include <cstddef>

namespace HYP36ROutputExposure
{
	constexpr size_t RecentPeakSamples = 30;
	constexpr size_t SustainedOneSecondSamples = 60;
	constexpr size_t SustainedThreeSecondSamples = 180;

	struct Frame
	{
		float composerInput = 0.0f;
		float postTanh = 0.0f;
		float instantaneousMagnitude = 0.0f;
		float normalizedHeadroom = 1.0f;
		float recentPeak = 0.0f;
		float sustainedOneSecond = 0.0f;
		float sustainedThreeSeconds = 0.0f;
		float occupancy50 = 0.0f;
		float occupancy75 = 0.0f;
		float occupancy90 = 0.0f;
		float occupancy98 = 0.0f;
		float outputSlewPerSecond = 0.0f;
		bool preTanhOverUnity = false;
		bool nearBoundary = false;
		bool directInputClampActive = false;
	};

	class Observer
	{
		std::array<float, SustainedThreeSecondSamples> magnitudes_{};
		size_t nextSample_ = 0;
		size_t sampleCount_ = 0;
		float previousPreDrive_ = 0.0f;
		bool havePrevious_ = false;
		Frame current_{};

	public:
		const Frame& observe(float composerInput, float postTanh, float preDriveOutput,
			float deltaSeconds);
		void observe_directinput_clamp(bool active);
		void reset();
		const Frame& frame() const { return current_; }
	};

	const Frame& observe(float composerInput, float postTanh, float preDriveOutput,
		float deltaSeconds);
	void observe_directinput_clamp(bool active);
	void reset();
	const Frame& frame();
}
