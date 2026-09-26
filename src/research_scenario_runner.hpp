#pragma once

#include <array>
#include <cstddef>
#include <span>

namespace HYP36RResearchRunner
{
	struct Scenario
	{
		const char* id;
		const char* name;
		const char* instruction;
		const char* avoidance;
		const char* notes;
		double durationSeconds;
	};

	inline constexpr std::array<Scenario, 7> R2AScenarios{{
		{ "R2_A01_STRAIGHT_BASELINE", "A01 Straight Baseline", "Drive straight at a steady speed", "Avoid steering and surface transitions", "R2-A vehicle/Force baseline", 20.0 },
		{ "R2_A02_PROGRESSIVE_LEFT", "A02 Progressive Left", "Progressively load a clean left corner", "Stay on asphalt; do not drift", "R2-A vehicle/Force baseline", 18.0 },
		{ "R2_A03_PROGRESSIVE_RIGHT", "A03 Progressive Right", "Progressively load a clean right corner", "Stay on asphalt; do not drift", "R2-A vehicle/Force baseline", 18.0 },
		{ "R2_A04_SUSTAINED_HIGH_LOAD_CORNER", "A04 Sustained High-Load Corner", "Hold steering load through a long corner", "Stay in grip and avoid runoff", "R2-A vehicle/Force baseline", 20.0 },
		{ "R2_A05_DRIFT_INITIATION", "A05 Drift Initiation", "Transition cleanly from grip into drift", "Prioritize the transition, not duration", "R2-A vehicle/Force baseline", 15.0 },
		{ "R2_A06_SUSTAINED_DRIFT", "A06 Sustained Drift", "Establish and hold a stable drift", "Avoid impacts where practical", "R2-A vehicle/Force baseline", 18.0 },
		{ "R2_A07_RELEASE_RECOVERY", "A07 Release / Recovery", "Recover from slip or drift into grip", "Prioritize recovery and BITE", "R2-A vehicle/Force baseline", 15.0 },
	}};

	inline constexpr std::array<Scenario, 6> R2BScenarios{{
		{ "R2_B01_LOCAL_ASPHALT_CONTROL", "B01 Local Asphalt Control", "Drive only on local normal asphalt", "Avoid curbs, runoff, impacts and drift", "R2-B local group 1: use the same area for B01/B02/B03", 20.0 },
		{ "R2_B02_STRIPED_RUNOFF_PARTIAL", "B02 Striped Runoff Partial", "Put one side on striped runoff, then return", "Keep the other side on asphalt; avoid drift", "R2-B local group 1: use the same area for B01/B02/B03", 18.0 },
		{ "R2_B03_STRIPED_RUNOFF_FULL", "B03 Striped Runoff Full", "Move most or all of the car onto runoff", "Use the B01/B02 area; avoid impacts", "R2-B local group 1: use the same area for B01/B02/B03", 18.0 },
		{ "R2_B04_ROUGH_SAND_PARTIAL", "B04 Rough / Sand Partial", "Put one side on the rough target, then return", "Keep the other side on asphalt; avoid drift", "R2-B local group 2: use the same target for B04/B05/B06", 18.0 },
		{ "R2_B05_ROUGH_SAND_FULL", "B05 Rough / Sand Full", "Move most or all of the car onto the target", "Use the B04 target; avoid impacts", "R2-B local group 2: use the same target for B04/B05/B06", 18.0 },
		{ "R2_B06_SURFACE_REENTRY", "B06 Surface Re-entry", "Begin off-surface and return cleanly to asphalt", "Avoid unnecessary impact", "R2-B local group 2: use the same target for B04/B05/B06", 18.0 },
	}};

	inline constexpr std::array<Scenario, 2> R2CScenarios{{
		{ "R2_C01_GEAR_SHIFTS", "C01 Gear Shifts", "Drive cleanly and perform several ordinary gear changes", "Stay on normal asphalt; avoid impacts, runoff and drift", "R2-C: isolate ordinary upshifts and downshifts on normal asphalt", 25.0 },
		{ "R2_C02_CONTROLLED_IMPACT", "C02 Controlled Impact", "Create separated mild and moderate contact events", "Do not maximize force, scrape continuously or combine with rough terrain", "R2-C: discrete controlled impacts separated by clean driving", 20.0 },
	}};

	enum class Phase { Ready, Countdown, Capturing, Review, Finished };
	enum class Action { None, StartCapture, StopCapture };

	class Runner
	{
		std::span<const Scenario> scenarios_;
		std::size_t scenarioIndex_ = 0;
		unsigned attempt_ = 1;
		Phase phase_ = Phase::Ready;
		double phaseStarted_ = 0.0;
		double actualDuration_ = 0.0;

	public:
		explicit Runner(std::span<const Scenario> scenarios) : scenarios_(scenarios) {}
		const Scenario& scenario() const { return scenarios_[scenarioIndex_]; }
		std::span<const Scenario> scenarios() const { return scenarios_; }
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
			if (scenarioIndex_ + 1 == scenarios_.size())
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

		bool cancel(bool captureStarted = false)
		{
			if (phase_ != Phase::Countdown && phase_ != Phase::Capturing)
				return false;
			if (captureStarted)
				++attempt_;
			phase_ = Phase::Ready;
			return true;
		}

		double phase_elapsed(double now) const
		{
			return (now > phaseStarted_) ? now - phaseStarted_ : 0.0;
		}
	};
}
