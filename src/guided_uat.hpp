#pragma once

#include <array>
#include <cstddef>

namespace GuidedUat
{
	inline constexpr const char Protocol[] = "UAT_ROAD_CALIBRATION_SWEEP_V1";
	inline constexpr double WarmupSeconds = 15.0;
	inline constexpr double CaptureSeconds = 36.0;
	inline constexpr std::array<int, 6> RoadCalibrationStages{ 8, 10, 15, 20, 25, 30 };

	enum class Phase { Idle, Instructions, WaitingForConfiguration, Warmup, Recording, Assessment, Complete };
	enum class Assessment { Unset, TooShallow, Legible, Good, TooStrong };

	struct StageResult
	{
		int requiredMultiplier = 0;
		int actualMultiplier = 0;
		double measuredDuration = 0.0;
		bool configurationMismatch = false;
		Assessment assessment = Assessment::Unset;
	};

	class RoadCalibrationSweep
	{
	public:
		void start() noexcept;
		void cancel() noexcept;
		bool begin_stage(int activeMultiplier, bool requiredSettingsMatch, double now) noexcept;
		bool start_capture_now(int activeMultiplier, double now) noexcept;
		void update(int activeMultiplier, double now) noexcept;
		void finish_capture(double now) noexcept;
		void assess(Assessment value) noexcept;
		void retry() noexcept;
		void set_preference(int multiplier) noexcept;
		void finish() noexcept;

		Phase phase() const noexcept { return phase_; }
		bool active() const noexcept { return phase_ != Phase::Idle; }
		std::size_t stage_index() const noexcept { return stageIndex_; }
		int required_multiplier() const noexcept { return RoadCalibrationStages[stageIndex_]; }
		double phase_elapsed(double now) const noexcept { return now > phaseStarted_ ? now - phaseStarted_ : 0.0; }
		double warmup_remaining(double now) const noexcept;
		double capture_elapsed(double now) const noexcept;
		bool capture_complete(double now) const noexcept;
		bool current_mismatch() const noexcept { return results_[stageIndex_].configurationMismatch; }
		int preferred_multiplier() const noexcept { return preferredMultiplier_; }
		const std::array<StageResult, 6>& results() const noexcept { return results_; }

	private:
		void begin_recording(int activeMultiplier, double now) noexcept;
		Phase phase_ = Phase::Idle;
		std::size_t stageIndex_ = 0;
		double phaseStarted_ = 0.0;
		int preferredMultiplier_ = 0;
		std::array<StageResult, 6> results_{};
	};

	const char* assessment_name(Assessment value) noexcept;
	void request_road_calibration_sweep() noexcept;
	bool consume_road_calibration_sweep_request() noexcept;
}
