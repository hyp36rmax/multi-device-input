#pragma once

#include <array>
#include <cstddef>

namespace HYP36RResearchRunner
{
	struct Scenario
	{
		const char* id;
		const char* name;
		const char* instruction;
		const char* avoidance;
		double durationSeconds;
	};

	inline constexpr std::array<Scenario, 7> Scenarios{{
		{ "R2_A01_STRAIGHT_BASELINE", "A01 Straight Baseline", "Drive straight at a steady speed", "Avoid steering and surface transitions", 20.0 },
		{ "R2_A02_PROGRESSIVE_LEFT", "A02 Progressive Left", "Progressively load a clean left corner", "Stay on asphalt; do not drift", 18.0 },
		{ "R2_A03_PROGRESSIVE_RIGHT", "A03 Progressive Right", "Progressively load a clean right corner", "Stay on asphalt; do not drift", 18.0 },
		{ "R2_A04_SUSTAINED_HIGH_LOAD_CORNER", "A04 Sustained High-Load Corner", "Hold steering load through a long corner", "Stay in grip and avoid runoff", 20.0 },
		{ "R2_A05_DRIFT_INITIATION", "A05 Drift Initiation", "Transition cleanly from grip into drift", "Prioritize the transition, not duration", 15.0 },
		{ "R2_A06_SUSTAINED_DRIFT", "A06 Sustained Drift", "Establish and hold a stable drift", "Avoid impacts where practical", 18.0 },
		{ "R2_A07_RELEASE_RECOVERY", "A07 Release / Recovery", "Recover from slip or drift into grip", "Prioritize recovery and BITE", 15.0 },
	}};

	enum class Phase { Ready, Countdown, Capturing, Review, Finished };
	enum class Action { None, StartCapture, StopCapture };

	class Runner
	{
		std::size_t scenarioIndex_ = 0;
		unsigned attempt_ = 1;
		Phase phase_ = Phase::Ready;
		double phaseStarted_ = 0.0;
		double actualDuration_ = 0.0;

	public:
		const Scenario& scenario() const { return Scenarios[scenarioIndex_]; }
		std::size_t scenario_index() const { return scenarioIndex_; }
		unsigned attempt() const { return attempt_; }
		Phase phase() const { return phase_; }
		double actual_duration() const { return actualDuration_; }

		bool start(double now)
		{
			if (phase_ != Phase::Ready)
				return false;
			phase_ = Phase::Countdown;
			phaseStarted_ = now;
			actualDuration_ = 0.0;
			return true;
		}

		Action update(double now)
		{
			if (phase_ == Phase::Countdown && now - phaseStarted_ >= 3.0)
			{
				phase_ = Phase::Capturing;
				phaseStarted_ = now;
				return Action::StartCapture;
			}
			if (phase_ == Phase::Capturing)
			{
				actualDuration_ = (now - phaseStarted_ > 0.0) ? now - phaseStarted_ : 0.0;
				if (actualDuration_ >= scenario().durationSeconds)
				{
					phase_ = Phase::Review;
					return Action::StopCapture;
				}
			}
			return Action::None;
		}

		bool accept()
		{
			if (phase_ != Phase::Review)
				return false;
			if (scenarioIndex_ + 1 == Scenarios.size())
				phase_ = Phase::Finished;
			else
			{
				++scenarioIndex_;
				attempt_ = 1;
				phase_ = Phase::Ready;
			}
			return true;
		}

		bool retry(double now)
		{
			if (phase_ != Phase::Review)
				return false;
			++attempt_;
			phase_ = Phase::Ready;
			return start(now);
		}

		bool cancel()
		{
			if (phase_ != Phase::Countdown && phase_ != Phase::Capturing)
				return false;
			phase_ = Phase::Ready;
			return true;
		}

		double phase_elapsed(double now) const
		{
			return (now > phaseStarted_) ? now - phaseStarted_ : 0.0;
		}
	};
}
