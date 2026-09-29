#pragma once

#include <array>
#include <cstddef>

namespace GuidedUat
{
	inline constexpr const char Protocol[] = "UAT_ROAD_CALIBRATION_SWEEP_V1";
	inline constexpr double WarmupSeconds = 15.0;
	inline constexpr double CaptureSeconds = 36.0;
	inline constexpr std::array<int, 6> RoadCalibrationStages{ 8, 10, 15, 20, 25, 30 };
	inline constexpr std::array<int, 7> SurfaceAmplitudeStages{ 12, 18, 24, 30, 36, 42, 50 };
	inline constexpr std::array<int, 3> SurfaceWaveformStages{ 0, 1, 2 };
	inline constexpr std::array<int, 4> SurfaceFrequencyStages{ 0, 1, 2, 3 };
	enum class SurfaceProtocol { None, Amplitude, Waveform, Frequency };
	struct ResolvedPreference { int value = 0; bool inherited = false; };
	ResolvedPreference resolve_amplitude_preference(int storedValue) noexcept;
	ResolvedPreference resolve_waveform_preference(int storedValue, bool isSet) noexcept;

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

	class SurfaceSweep
	{
	public:
		void start(SurfaceProtocol protocol) noexcept;
		void cancel() noexcept { phase_ = Phase::Idle; }
		bool begin_stage(int actualValue, bool constantsMatch, double now) noexcept;
		bool start_capture_now(int actualValue, double now) noexcept;
		void update(int actualValue, bool constantsMatch, double now) noexcept;
		void finish_capture(double now) noexcept;
		void assess(int value) noexcept;
		void retry() noexcept;
		void set_preference(int value) noexcept { preferredValue_ = valid_stage(value) ? value : -1; }
		void finish() noexcept { if (phase_ == Phase::Complete && preferredValue_ >= 0) phase_ = Phase::Idle; }
		SurfaceProtocol protocol() const noexcept { return protocol_; }
		Phase phase() const noexcept { return phase_; }
		bool active() const noexcept { return phase_ != Phase::Idle; }
		std::size_t stage_index() const noexcept { return stageIndex_; }
		std::size_t stage_count() const noexcept;
		int required_value() const noexcept;
		int preferred_value() const noexcept { return preferredValue_; }
		bool current_mismatch() const noexcept { return mismatch_; }
		double warmup_remaining(double now) const noexcept;
		double capture_elapsed(double now) const noexcept;
		bool capture_complete(double now) const noexcept;
		int assessment(std::size_t index) const noexcept { return assessments_[index]; }
	private:
		bool valid_stage(int value) const noexcept;
		SurfaceProtocol protocol_ = SurfaceProtocol::None;
		Phase phase_ = Phase::Idle;
		std::size_t stageIndex_ = 0;
		double phaseStarted_ = 0.0;
		int preferredValue_ = -1;
		bool mismatch_ = false;
		std::array<int, 7> assessments_{};
	};

	const char* assessment_name(Assessment value) noexcept;
	void request_road_calibration_sweep() noexcept;
	bool consume_road_calibration_sweep_request() noexcept;
	void request_surface_sweep(SurfaceProtocol protocol) noexcept;
	SurfaceProtocol consume_surface_sweep_request() noexcept;
}
