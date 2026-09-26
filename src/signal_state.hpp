#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace HYP36RSignalState
{
	inline constexpr uint32_t SchemaVersion = 1;
	inline constexpr uint32_t InterpretationVersion = 1;
	inline constexpr const char SchemaName[] = "HYP36R_SIGNAL_STATE_V1";
	inline constexpr size_t CornerCount = 4;

	enum class Validity : uint8_t { Unavailable, Valid, Stale, Nonfinite, UnsupportedSource };
	enum class Source : uint8_t {
		None, CurrentRuntime, NativeVehicleState, NativeSurfaceState,
		NativeFourCorner, RestoredNativeEffect, DerivedEvidenceSupported
	};
	enum class Confidence : uint8_t { Unknown, Possible, Moderate, High };
	enum class FallbackReason : uint8_t { None, Missing, Stale, Nonfinite, UnsupportedSource };
	enum class GripState : uint8_t { Unavailable, Load, Release, Free, Recovering, Bite };
	enum class EventClass : uint8_t { None, GearShift, CollisionCandidate, UnknownNativeEvent };

	struct SignalMeta
	{
		Validity validity = Validity::Unavailable;
		Source source = Source::None;
		Confidence confidence = Confidence::Unknown;
		FallbackReason fallback = FallbackReason::Missing;
		uint64_t sampleFrame = 0;
		uint32_t ageFrames = 0;
	};

	struct VehicleObservation
	{
		SignalMeta meta{};
		float steering = 0.0f;
		float speed = 0.0f;
		float normalizedSpeed = 0.0f;
		float responseAngle = 0.0f;
		float responseRate = 0.0f;
		float referenceResponseError = 0.0f;
		float responseAuthority = 0.0f;
	};

	struct SurfaceObservation
	{
		SignalMeta meta{};
		std::array<uint32_t, CornerCount> current{};
		std::array<uint32_t, CornerCount> previous{};
		std::array<bool, CornerCount> changed{};
		// field_14 is validation/provenance only. It is intentionally not copied
		// into a second consumer-facing surface signal.
		bool field14ValidationAvailable = false;
		bool field14MatchesCanonical = false;
		uint8_t field14MismatchMask = 0;
	};

	struct SpatialOccupancy
	{
		bool allSame = true;
		bool mixed = false;
		uint8_t distinctCount = 0;
		bool pair02Same = true;
		bool pair13Same = true;
		// Corner-to-wheel ownership is not yet established, so no left/right
		// semantics are asserted by this contract.
		bool wheelOwnershipKnown = false;
	};

	struct NativeEffectObservation
	{
		SignalMeta meta{};
		float left = 0.0f;
		float right = 0.0f;
		float combined = 0.0f;
		float rise = 0.0f;
	};

	struct GearEvent
	{
		SignalMeta meta{};
		int current = 0;
		int previous = 0;
		bool transition = false;
	};

	struct GripObservation
	{
		SignalMeta meta{};
		GripState state = GripState::Unavailable;
	};

	struct NativeDynamicCandidates
	{
		SignalMeta meta{};
		std::array<float, CornerCount> fieldE8{};
		std::array<int16_t, CornerCount> fieldEC{};
		std::array<int16_t, CornerCount> fieldEE{};
	};

	struct RoadShadowIntent
	{
		SignalMeta meta{};
		SpatialOccupancy occupancy{};
		float continuousActivityEvidence = 0.0f;
		bool surfaceTransitionObserved = false;
	};

	struct EventShadowIntent
	{
		SignalMeta meta{};
		EventClass classification = EventClass::None;
		float leftEvidence = 0.0f;
		float rightEvidence = 0.0f;
		float riseEvidence = 0.0f;
		float existingImpactEvidence = 0.0f;
		bool gearTransition = false;
	};

	struct Frame
	{
		uint32_t schemaVersion = SchemaVersion;
		uint32_t interpretationVersion = InterpretationVersion;
		uint64_t frameId = 0;
		VehicleObservation vehicle{};
		SurfaceObservation surfaces{};
		SpatialOccupancy occupancy{};
		NativeEffectObservation nativeEffect{};
		GearEvent gear{};
		GripObservation grip{};
		NativeDynamicCandidates dynamics{};
		RoadShadowIntent roadIntent{};
		EventShadowIntent eventIntent{};
	};

	struct Inputs
	{
		uint64_t frameId = 0;
		bool sourceSupported = true;
		float steering = 0.0f;
		float speed = 0.0f;
		float normalizedSpeed = 0.0f;
		bool nativeVehicleValid = false;
		float responseAngle = 0.0f;
		float responseRate = 0.0f;
		float referenceResponseError = 0.0f;
		float responseAuthority = 0.0f;
		std::array<uint32_t, CornerCount> surfaces{};
		bool field14Available = false;
		std::array<uint32_t, CornerCount> field14{};
		float effectLeft = 0.0f;
		float effectRight = 0.0f;
		float effectCombined = 0.0f;
		float effectRise = 0.0f;
		float existingImpact = 0.0f;
		int currentGear = 0;
		int previousGear = 0;
		bool gearTransition = false;
		GripState gripState = GripState::Unavailable;
		bool nativeDynamicsAvailable = false;
		std::array<float, CornerCount> fieldE8{};
		std::array<int16_t, CornerCount> fieldEC{};
		std::array<int16_t, CornerCount> fieldEE{};
	};

	struct KnownGearFixture
	{
		static constexpr float Left = 0.14f;
		static constexpr float Right = 0.0f;
		static constexpr float Rise = 0.14f;
		static constexpr float ExistingImpact = 0.07735f;
	};

	class PassiveObserver
	{
	public:
		const Frame& observe(const Inputs& inputs) noexcept;
		Frame snapshot(uint64_t currentFrame, uint32_t maxAgeFrames = 2) const noexcept;
		void reset() noexcept;
		const Frame& frame() const noexcept { return current_; }

	private:
		Frame current_{};
		std::array<uint32_t, CornerCount> previousSurfaces_{};
		bool havePreviousSurfaces_ = false;
	};

	// Runtime ownership: update/reset are called only by the player-car control
	// thread. Readers on other threads require external synchronization. The
	// fixed-size frame performs no allocation, file I/O, or string work.
	const Frame& update(const Inputs& inputs) noexcept;
	Frame snapshot(uint64_t currentFrame, uint32_t maxAgeFrames = 2) noexcept;
	void reset() noexcept;
	const Frame& frame() noexcept;
}
