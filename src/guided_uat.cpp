#include "guided_uat.hpp"

#include <algorithm>

namespace GuidedUat
{
	namespace { bool roadSweepRequested = false; }

	void RoadCalibrationSweep::start() noexcept
	{
		phase_ = Phase::Instructions;
		stageIndex_ = 0;
		phaseStarted_ = 0.0;
		preferredMultiplier_ = 0;
		results_ = {};
		for (std::size_t i = 0; i < results_.size(); ++i)
			results_[i].requiredMultiplier = RoadCalibrationStages[i];
	}

	void RoadCalibrationSweep::cancel() noexcept { phase_ = Phase::Idle; }

	bool RoadCalibrationSweep::begin_stage(int activeMultiplier, bool requiredSettingsMatch, double now) noexcept
	{
		if ((phase_ != Phase::Instructions && phase_ != Phase::WaitingForConfiguration) ||
			!requiredSettingsMatch || activeMultiplier != required_multiplier())
		{
			phase_ = Phase::WaitingForConfiguration;
			return false;
		}
		results_[stageIndex_].actualMultiplier = activeMultiplier;
		results_[stageIndex_].configurationMismatch = false;
		results_[stageIndex_].measuredDuration = 0.0;
		results_[stageIndex_].assessment = Assessment::Unset;
		phaseStarted_ = now;
		phase_ = Phase::Warmup;
		return true;
	}

	void RoadCalibrationSweep::begin_recording(int activeMultiplier, double now) noexcept
	{
		results_[stageIndex_].actualMultiplier = activeMultiplier;
		results_[stageIndex_].configurationMismatch = activeMultiplier != required_multiplier();
		phaseStarted_ = now;
		phase_ = Phase::Recording;
	}

	bool RoadCalibrationSweep::start_capture_now(int activeMultiplier, double now) noexcept
	{
		if (phase_ != Phase::Warmup || activeMultiplier != required_multiplier())
			return false;
		begin_recording(activeMultiplier, now);
		return true;
	}

	void RoadCalibrationSweep::update(int activeMultiplier, double now) noexcept
	{
		if (phase_ == Phase::WaitingForConfiguration && activeMultiplier == required_multiplier())
			phase_ = stageIndex_ == 0 ? Phase::Instructions : Phase::WaitingForConfiguration;
		if (phase_ == Phase::Recording)
		{
			results_[stageIndex_].actualMultiplier = activeMultiplier;
			if (activeMultiplier != required_multiplier())
				results_[stageIndex_].configurationMismatch = true;
		}
	}

	void RoadCalibrationSweep::finish_capture(double now) noexcept
	{
		if (phase_ != Phase::Recording) return;
		results_[stageIndex_].measuredDuration = (std::min)(CaptureSeconds, phase_elapsed(now));
		phase_ = Phase::Assessment;
	}

	void RoadCalibrationSweep::assess(Assessment value) noexcept
	{
		if (phase_ != Phase::Assessment || value == Assessment::Unset) return;
		results_[stageIndex_].assessment = value;
		if (++stageIndex_ >= RoadCalibrationStages.size())
		{
			stageIndex_ = RoadCalibrationStages.size() - 1;
			phase_ = Phase::Complete;
		}
		else phase_ = Phase::WaitingForConfiguration;
	}

	void RoadCalibrationSweep::retry() noexcept
	{
		if (phase_ != Phase::Assessment) return;
		results_[stageIndex_] = {};
		results_[stageIndex_].requiredMultiplier = required_multiplier();
		phase_ = Phase::WaitingForConfiguration;
	}

	void RoadCalibrationSweep::set_preference(int multiplier) noexcept
	{
		for (int candidate : RoadCalibrationStages)
			if (candidate == multiplier) preferredMultiplier_ = multiplier;
	}

	void RoadCalibrationSweep::finish() noexcept
	{
		if (phase_ == Phase::Complete && preferredMultiplier_ != 0) phase_ = Phase::Idle;
	}

	double RoadCalibrationSweep::warmup_remaining(double now) const noexcept
	{
		return phase_ == Phase::Warmup ? (std::max)(0.0, WarmupSeconds - phase_elapsed(now)) : 0.0;
	}

	double RoadCalibrationSweep::capture_elapsed(double now) const noexcept
	{
		return phase_ == Phase::Recording ? (std::min)(CaptureSeconds, phase_elapsed(now)) : 0.0;
	}

	bool RoadCalibrationSweep::capture_complete(double now) const noexcept
	{
		return phase_ == Phase::Recording && phase_elapsed(now) >= CaptureSeconds;
	}

	const char* assessment_name(Assessment value) noexcept
	{
		switch (value)
		{
		case Assessment::TooShallow: return "Too Shallow";
		case Assessment::Legible: return "Legible";
		case Assessment::Good: return "Good";
		case Assessment::TooStrong: return "Too Strong";
		default: return "Not assessed";
		}
	}

	void request_road_calibration_sweep() noexcept { roadSweepRequested = true; }
	bool consume_road_calibration_sweep_request() noexcept
	{
		const bool requested = roadSweepRequested;
		roadSweepRequested = false;
		return requested;
	}
}
