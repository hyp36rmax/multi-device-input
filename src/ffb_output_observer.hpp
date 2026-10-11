#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace HYP36RFFBOutput
{
	enum class EffectType { Unavailable, Constant, Sine, Triangle, Square, Bump };
	enum class Strategy { Unavailable, TwoAxisPolar, OneAxisCartesianRecreation, PeriodicPersistent, OneShotBump };
	enum class Operation { None, Acquire, CreateEffect, SetParameters, Start, Stop };
	enum class SafetyState { Unavailable, Ready, Disabled, FocusLost, Watchdog, Stopped };
	enum class TimingClass { Unavailable, Comparable, IdleGap, NonMonotonic };

	struct Submission
	{
		float finalRequestedForce = 0.0f;
		int32_t requestedMagnitude = 0;
		int32_t requestedDirection = 0;
		EffectType effect = EffectType::Unavailable;
		Strategy strategy = Strategy::Unavailable;
		uint32_t actuatorAxes = 0;
		bool deviceReady = false;
		Operation operation = Operation::None;
		int32_t result = 0;
		uint64_t timestampUs = 0;
		uint64_t requestedIntervalUs = 0;
		bool recreation = false;
		bool persistentUpdate = false;
	};

	struct Frame
	{
		bool enabled = false;
		bool finalRequestedAvailable = false;
		float finalRequestedForce = 0.0f;
		bool directInputRequestAvailable = false;
		int32_t requestedMagnitude = 0;
		int32_t requestedDirection = 0;
		EffectType effect = EffectType::Unavailable;
		Strategy strategy = Strategy::Unavailable;
		uint32_t actuatorAxes = 0;
		bool deviceReady = false;
		SafetyState safety = SafetyState::Unavailable;
		Operation operation = Operation::None;
		bool resultAvailable = false;
		int32_t result = 0;
		uint64_t submissionTimestampUs = 0;
		uint64_t submissionIntervalUs = 0;
		uint64_t requestedIntervalUs = 0;
		int64_t timingJitterUs = 0;
		TimingClass timingClass = TimingClass::Unavailable;
		uint64_t recreationCount = 0;
		uint64_t persistentUpdateCount = 0;
		uint64_t watchdogShutdownCount = 0;
		uint64_t bufferedSamples = 0;
		uint64_t droppedSamples = 0;
		uint64_t recordedDirectInputOperations = 0;
		uint64_t failedDirectInputOperations = 0;
	};

	class Observer
	{
	public:
		static constexpr size_t Capacity = 256;

		void set_enabled(bool enabled)
		{
			frame_.enabled = enabled;
			if (!enabled) frame_.safety = SafetyState::Disabled;
		}

		void record(const Submission& value)
		{
			if (!frame_.enabled) return;
			retain(value);
			count_operation(value.operation, value.result);
			frame_.finalRequestedAvailable = true;
			frame_.finalRequestedForce = value.finalRequestedForce;
			frame_.directInputRequestAvailable = true;
			frame_.requestedMagnitude = value.requestedMagnitude;
			frame_.requestedDirection = value.requestedDirection;
			frame_.effect = value.effect;
			frame_.strategy = value.strategy;
			frame_.actuatorAxes = value.actuatorAxes;
			frame_.deviceReady = value.deviceReady;
			frame_.operation = value.operation;
			frame_.resultAvailable = value.operation != Operation::None;
			frame_.result = value.result;
			frame_.safety = value.deviceReady ? SafetyState::Ready : SafetyState::Stopped;
			frame_.timingClass = TimingClass::Unavailable;
			frame_.submissionIntervalUs = 0;
			frame_.timingJitterUs = 0;
			if (value.timestampUs != 0)
			{
				if (lastTimestampUs_ != 0 && value.timestampUs < lastTimestampUs_)
					frame_.timingClass = TimingClass::NonMonotonic;
				else if (lastTimestampUs_ != 0)
					frame_.submissionIntervalUs = value.timestampUs - lastTimestampUs_;
				frame_.submissionTimestampUs = value.timestampUs;
				if (value.timestampUs >= lastTimestampUs_) lastTimestampUs_ = value.timestampUs;
			}
			frame_.requestedIntervalUs = value.requestedIntervalUs;
			if (frame_.submissionIntervalUs && value.requestedIntervalUs)
			{
				frame_.timingJitterUs = static_cast<int64_t>(frame_.submissionIntervalUs) -
					static_cast<int64_t>(value.requestedIntervalUs);
				frame_.timingClass = frame_.submissionIntervalUs > value.requestedIntervalUs * 4
					? TimingClass::IdleGap : TimingClass::Comparable;
			}
			if (value.recreation) ++frame_.recreationCount;
			if (value.persistentUpdate) ++frame_.persistentUpdateCount;
		}

		void record_api(Operation operation, int32_t result, uint64_t timestampUs,
			EffectType effect, Strategy strategy, uint32_t axes, bool ready)
		{
			if (!frame_.enabled) return;
			const Submission event{ .effect = effect, .strategy = strategy,
				.actuatorAxes = axes, .deviceReady = ready, .operation = operation,
				.result = result, .timestampUs = timestampUs };
			retain(event);
			count_operation(operation, result);
			frame_.operation = operation; frame_.resultAvailable = true; frame_.result = result;
			frame_.effect = effect; frame_.strategy = strategy; frame_.actuatorAxes = axes;
			frame_.deviceReady = ready; frame_.submissionTimestampUs = timestampUs;
		}

		void safety(SafetyState state)
		{
			if (!frame_.enabled) return;
			frame_.safety = state;
			if (state == SafetyState::Watchdog) ++frame_.watchdogShutdownCount;
		}

		const Frame& frame() const { return frame_; }
		std::vector<Submission> retained_events() const
		{
			std::vector<Submission> result;
			result.reserve(count_);
			const size_t first = count_ == Capacity ? head_ : 0;
			for (size_t index = 0; index < count_; ++index)
				result.push_back(buffer_[(first + index) % Capacity]);
			return result;
		}
		void reset() { *this = {}; }

	private:
		void retain(const Submission& value)
		{
			if (count_ == Capacity) ++frame_.droppedSamples;
			else ++count_;
			buffer_[head_] = value;
			head_ = (head_ + 1) % Capacity;
			frame_.bufferedSamples = count_;
		}

		void count_operation(Operation operation, int32_t result)
		{
			if (operation == Operation::None) return;
			++frame_.recordedDirectInputOperations;
			if (result < 0) ++frame_.failedDirectInputOperations;
		}

		// All production callers belong to the wheel-output/game update thread.
		// Readers consume the published Frame on that same UI/game thread; no lock
		// or allocation occurs in record() or record_api().
		std::array<Submission, Capacity> buffer_{};
		size_t head_ = 0;
		size_t count_ = 0;
		uint64_t lastTimestampUs_ = 0;
		Frame frame_{};
	};

	inline Observer observer;
	inline void set_enabled(bool enabled) { observer.set_enabled(enabled); }
	inline void record(const Submission& value) { observer.record(value); }
	inline void record_api(Operation operation, int32_t result, uint64_t timestampUs,
		EffectType effect, Strategy strategy, uint32_t axes, bool ready)
	{ observer.record_api(operation, result, timestampUs, effect, strategy, axes, ready); }
	inline void safety(SafetyState state) { observer.safety(state); }
	inline const Frame& frame() { return observer.frame(); }

	constexpr std::string_view name(EffectType v) { switch (v) {
		case EffectType::Constant: return "Constant"; case EffectType::Sine: return "Sine";
		case EffectType::Triangle: return "Triangle"; case EffectType::Square: return "Square";
		case EffectType::Bump: return "Bump"; default: return "Unavailable"; } }
	constexpr std::string_view name(Strategy v) { switch (v) {
		case Strategy::TwoAxisPolar: return "Two-Axis Polar";
		case Strategy::OneAxisCartesianRecreation: return "One-Axis Cartesian / Recreation";
		case Strategy::PeriodicPersistent: return "Periodic Persistent";
		case Strategy::OneShotBump: return "One-Shot Bump"; default: return "Unavailable"; } }
	constexpr std::string_view name(Operation v) { switch (v) {
		case Operation::Acquire: return "Acquire"; case Operation::CreateEffect: return "CreateEffect";
		case Operation::SetParameters: return "SetParameters"; case Operation::Start: return "Start";
		case Operation::Stop: return "Stop"; default: return "None"; } }
	constexpr std::string_view name(SafetyState v) { switch (v) {
		case SafetyState::Ready: return "Ready"; case SafetyState::Disabled: return "Disabled";
		case SafetyState::FocusLost: return "Focus Lost"; case SafetyState::Watchdog: return "Watchdog";
		case SafetyState::Stopped: return "Stopped"; default: return "Unavailable"; } }
	constexpr std::string_view name(TimingClass v) { switch (v) {
		case TimingClass::Comparable: return "Comparable"; case TimingClass::IdleGap: return "Idle Gap";
		case TimingClass::NonMonotonic: return "Non-Monotonic"; default: return "Unavailable"; } }
}
