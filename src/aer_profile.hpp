#pragma once

#include <array>
#include <cstdint>

#include "signal_state.hpp"

namespace HYP36RAer
{
	inline constexpr uint32_t TelemetrySchemaVersion = 1;
	inline constexpr const char TelemetrySchemaName[] = "HYP36R_AER_PROFILE_V1";

	enum class Profile : uint8_t { ReferencePlus, ArcadeExperienceExperimental };
	enum class OperatingMode : uint8_t { Shadow, Active };
	enum class EventClass : uint8_t { None, SurfaceTransition, ContactAsymmetry, Impact };

	struct AerProfileSettings
	{
		int strengthPercent = 100;
		int roadDetailPercent = 100;
		OperatingMode mode = OperatingMode::Active;
	};

	// Clean-room adapter for already verified OutRun 2006 signals. No Sega
	// addresses, request bytes, or firmware interpretations enter this contract.
	struct AerEvidenceFrame
	{
		uint64_t frameId = 0;
		bool valid = false;
		float steering = 0.0f;
		float normalizedSpeed = 0.0f;
		float responseAngle = 0.0f;
		float responseAuthority = 0.0f;
		float roadActivity = 0.0f;
		float impactEvidence = 0.0f;
		std::array<uint32_t, 4> surfaces{};
		std::array<bool, 4> surfaceChanged{};
		bool mixedContact = false;
	};

	struct ContinuousOutput { float requested = 0.0f; };
	struct EventOutput
	{
		EventClass classification = EventClass::None;
		float requested = 0.0f;
		float remainingSeconds = 0.0f;
		bool active = false;
	};

	struct AerTelemetry
	{
		uint32_t schemaVersion = TelemetrySchemaVersion;
		bool selected = false;
		bool activeOutput = false;
		bool evidenceValid = false;
		float continuousRequest = 0.0f;
		EventClass eventClass = EventClass::None;
		float eventRequest = 0.0f;
		float preLimit = 0.0f;
		float finalRequest = 0.0f;
		bool limited = false;
		bool safetyOpen = false;
		int strengthPercent = 100;
		int roadDetailPercent = 100;
	};

	struct Frame
	{
		AerEvidenceFrame evidence{};
		ContinuousOutput continuous{};
		EventOutput event{};
		AerTelemetry telemetry{};
	};

	struct SafetyInputs
	{
		bool profileSelected = false;
		bool inGameplay = false;
		bool ffbEnabled = false;
		bool deviceReady = false;
		bool focused = true;
	};

	Profile profile_from_int(int value) noexcept;
	const char* profile_name(Profile profile) noexcept;
	AerEvidenceFrame adapt_evidence(const HYP36RSignalState::Frame& source) noexcept;

	class AerContinuousInterpreter
	{
	public:
		ContinuousOutput evaluate(const AerEvidenceFrame& evidence, float dt) noexcept;
		void reset() noexcept { filtered_ = 0.0f; }
	private:
		float filtered_ = 0.0f;
	};

	class AerEventInterpreter
	{
	public:
		EventOutput evaluate(const AerEvidenceFrame& evidence, int roadDetailPercent, float dt) noexcept;
		void reset() noexcept;
	private:
		EventOutput current_{};
		float cooldownSeconds_ = 0.0f;
	};

	class AerComposer
	{
	public:
		float compose(float continuous, float event, float dt, bool& limited) noexcept;
		void reset() noexcept { previous_ = 0.0f; }
	private:
		float previous_ = 0.0f;
	};

	class AerSafetyGate
	{
	public:
		float apply(float request, const AerEvidenceFrame& evidence,
			const SafetyInputs& safety, float dt, bool& open) noexcept;
		void reset() noexcept { held_ = 0.0f; staleSeconds_ = 0.0f; }
	private:
		float held_ = 0.0f;
		float staleSeconds_ = 0.0f;
	};

	class Runtime
	{
	public:
		const Frame& evaluate(const AerEvidenceFrame& evidence, const AerProfileSettings& settings,
			const SafetyInputs& safety, float dt) noexcept;
		void reset() noexcept;
		const Frame& frame() const noexcept { return frame_; }
	private:
		AerContinuousInterpreter continuous_{};
		AerEventInterpreter events_{};
		AerComposer composer_{};
		AerSafetyGate safety_{};
		Frame frame_{};
	};

	const char* event_name(EventClass eventClass) noexcept;
}
