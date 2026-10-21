#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <cmath>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace DeviceDiagnostics
{
	inline constexpr int PhysicalOutputCeilingPercent = 20;
	inline constexpr auto MaximumRunTime = std::chrono::milliseconds(1500);
	inline constexpr auto WatchdogTimeout = std::chrono::milliseconds(250);
	inline constexpr auto BackendObservationTime = std::chrono::seconds(10);
	inline constexpr auto CompatibilityUpdatePeriod = std::chrono::milliseconds(66);
	inline constexpr std::array<int, 12> CompatibilitySignalPercent{ 0, 5, 10, 15, 20, 15, 10, 0, -10, -20, -10, 0 };
	// Conservative synthetic fixture used because the original RS50 V8 capture is
	// not part of this repository. It models one sustained direction only: a
	// gradual ramp, a short steady load, and a small reduction/restoration.
	inline constexpr auto LoadedSteeringUpdatePeriod = std::chrono::milliseconds(66);
	inline constexpr auto LoadedSteeringPreparationTime = std::chrono::seconds(6);
	inline constexpr auto LoadedSteeringMaximumRunTime = std::chrono::seconds(15);
	inline constexpr int LoadedSteeringActivityThreshold = 2500;
	inline constexpr std::array<int, 21> LoadedSteeringSignalPercent{
		0, 4, 8, 12, 16, 20, 20, 20, 20, 20, 18, 16, 14, 16, 18, 20, 20, 16, 12, 6, 0
	};
	inline constexpr std::string_view LoadedSteeringFixtureProvenance =
		"Synthetic conservative fixture; original Logitech RS50 V8 CSV unavailable";
	inline constexpr size_t LoadedTransitionBegin = 10;
	inline constexpr size_t LoadedTransitionEnd = 15;
	struct DeliveryRequest { bool right=false; int magnitudePercent=0; };
	inline constexpr DeliveryRequest delivery_request(size_t index)
	{
		const int value=CompatibilitySignalPercent[index];return {value>=0,value<0?-value:value};
	}
	inline constexpr std::array<std::string_view, 3> VisibleFfbActions{ "Test Left", "Test Right", "Hold to Shake" };
	inline constexpr bool effective_right(bool requestedRight, bool inverted) { return requestedRight != inverted; }
	inline constexpr int requested_nominal_magnitude(int requestedPercent)
	{
		return std::clamp(requestedPercent, 0, 100) * 100;
	}
	inline constexpr int safety_limited_magnitude(int requestedPercent)
	{
		return std::min(requested_nominal_magnitude(requestedPercent), PhysicalOutputCeilingPercent * 100);
	}
	inline std::filesystem::path exports_path(const std::filesystem::path& documents)
	{
		return documents / "HYP36rforce Device Diagnostics" / "Exports";
	}

	enum class ResultState { Completed, Failed, Unavailable, Untested, Running, Cancelled };
	enum class CompatibilityStrategy { LegacyRecreation, PersistentDynamic };
	enum class PhysicalConfirmation { Untested, Yes, No, Unsure };
	enum class ResponseQuality { Untested, Smooth, Chatter, Flutter, Clicking, Other };
	enum class LoadedSensation : uint32_t { Smooth=1u<<0, Pulsing=1u<<1, Flutter=1u<<2, Chatter=1u<<3, Clicking=1u<<4, Other=1u<<5, Unsure=1u<<6 };
	enum class LoadedObservationPhase : uint32_t { Loading=1u<<0, Sustained=1u<<1, Transition=1u<<2, Recovery=1u<<3, RampDown=1u<<4, Unsure=1u<<5 };
	inline constexpr uint32_t flag(LoadedSensation value){return static_cast<uint32_t>(value);}
	inline constexpr uint32_t flag(LoadedObservationPhase value){return static_cast<uint32_t>(value);}
	inline constexpr std::string_view LoadedPhysicalResponseUnsureId = "Unsure##LoadedPhysicalResponse";
	inline constexpr std::string_view LoadedSensationUnsureId = "Unsure##LoadedSensation";
	inline constexpr std::string_view LoadedObservationTimingUnsureId = "Unsure##LoadedObservationTiming";
	inline constexpr uint32_t LoadedSpecificObservationPhases = flag(LoadedObservationPhase::Loading)|flag(LoadedObservationPhase::Sustained)|flag(LoadedObservationPhase::Transition)|flag(LoadedObservationPhase::Recovery)|flag(LoadedObservationPhase::RampDown);
	inline constexpr uint32_t select_observation_phase(uint32_t selected,LoadedObservationPhase value,bool enabled)
	{
		const uint32_t chosen=flag(value);
		if(!enabled)return selected&~chosen;
		if(value==LoadedObservationPhase::Unsure)return chosen;
		return (selected&~flag(LoadedObservationPhase::Unsure))|chosen;
	}
	enum class CompatibilityClassification { Inconclusive, DynamicAccepted, DynamicRejected, DynamicAcceptedUnverified, DynamicAcceptedIneffective };

	struct CompatibilityResult
	{
		CompatibilityStrategy strategy = CompatibilityStrategy::LegacyRecreation;
		ResultState state = ResultState::Untested;
		bool simulated = true;
		int actuatorAxes = 0;
		int requestedRateHz = 15;
		int createCount = 0, startCount = 0, stopCount = 0, updateCount = 0, releaseCount = 0;
		int replacementCount = 0, finalCleanupCount = 0, failureCount = 0;
		int peakMagnitude = 0;
		double averageMagnitude = 0.0, rmsMagnitude = 0.0, averageIntervalMs = 0.0, jitterMs = 0.0, zeroTimeMs = 0.0;
		std::vector<long> apiResults;
		std::vector<double> intervalsMs;
		PhysicalConfirmation physical = PhysicalConfirmation::Untested;
		ResponseQuality quality = ResponseQuality::Untested;
		uint32_t sensations = 0;
		uint32_t observationPhases = 0;
		int repetitionCount = 0;
		std::vector<double> repetitionBoundariesMs;
		double activeDurationMs = 0.0;
		std::string steeringDevice;
		int steeringAxis = -1;
		double steeringPositionAtStart = 0.0;
		int steeringPeakDelta = 0;
		bool heldTurnRight = true;
		std::string startedUtc, completedUtc, note;
	};
	inline void finalize_compatibility_statistics(CompatibilityResult& result, const std::vector<int>& magnitudes);

	inline constexpr bool loaded_fixture_valid()
	{
		if (LoadedSteeringSignalPercent.front() != 0 || LoadedSteeringSignalPercent.back() != 0)
			return false;
		for (const int request : LoadedSteeringSignalPercent)
			if (request < 0 || request > PhysicalOutputCeilingPercent) return false;
		return LoadedSteeringSignalPercent[LoadedTransitionBegin] > LoadedSteeringSignalPercent[LoadedTransitionEnd - 2]
			&& LoadedSteeringSignalPercent[LoadedTransitionEnd] == PhysicalOutputCeilingPercent;
	}
	inline constexpr auto loaded_sequence_duration()
	{
		return LoadedSteeringSignalPercent.size() * LoadedSteeringUpdatePeriod;
	}
	inline constexpr size_t loaded_complete_repetitions(std::chrono::milliseconds duration)
	{
		return static_cast<size_t>(duration / loaded_sequence_duration());
	}

	inline std::optional<int> steering_axis_from_control(std::string_view control)
	{
		if(!control.starts_with("Axis "))return std::nullopt;
		int axis=-1;const char* begin=control.data()+5;const char* end=control.data()+control.size();
		const auto [next,error]=std::from_chars(begin,end,axis);
		if(error!=std::errc{}||next==begin||axis<0)return std::nullopt;
		return axis;
	}

	struct LoadedSteeringActivity
	{
		std::string deviceId;
		int axis=-1,baseline=0,current=0,peakDelta=0;
		bool baselineValid=false,available=false,observed=false;
		void select(std::string id,int selectedAxis,int raw)
		{
			deviceId=std::move(id);axis=selectedAxis;baseline=current=raw;peakDelta=0;baselineValid=true;available=true;observed=false;
		}
		void observe(std::string_view id,int selectedAxis,bool connected,int raw)
		{
			if(id!=deviceId||selectedAxis!=axis)return;
			if(!connected){available=false;observed=false;baselineValid=false;return;}
			available=true;current=raw;if(!baselineValid){baseline=raw;peakDelta=0;baselineValid=true;observed=false;return;}
			peakDelta=std::max(peakDelta,std::abs(raw-baseline));if(peakDelta>=LoadedSteeringActivityThreshold)observed=true;
		}
		void unavailable(){available=false;observed=false;baselineValid=false;}
		bool ready()const{return !deviceId.empty()&&axis>=0&&available&&baselineValid&&observed;}
		double normalized_position()const{return current<0?current/32768.0:current/32767.0;}
	};

	struct LoadedSafetyController
	{
		bool authorized=false;
		bool running=false;
		std::chrono::steady_clock::time_point started{},heartbeat{};
		int bounded_magnitude(int requestedPercent)const{return safety_limited_magnitude(requestedPercent);}
		bool begin(bool selected,bool steeringReady,bool focused,std::chrono::steady_clock::time_point now)
		{
			if(!authorized||!selected||!steeringReady||!focused)return false;
			running=true;started=heartbeat=now;return true;
		}
		void beat(std::chrono::steady_clock::time_point now){heartbeat=now;}
		bool must_stop(bool focused,bool connected,bool steeringReady,std::chrono::steady_clock::time_point now)const
		{
			return running&&(!authorized||!focused||!connected||!steeringReady||now-started>=LoadedSteeringMaximumRunTime||now-heartbeat>=WatchdogTimeout);
		}
		void stop(){running=false;authorized=false;}
	};

	inline CompatibilityResult simulate_loaded_steering(CompatibilityStrategy strategy, int actuatorAxes = 1, int failAt = -1)
	{
		CompatibilityResult result; result.strategy = strategy; result.state = ResultState::Running;
		result.actuatorAxes = actuatorAxes; result.requestedRateHz = 15;
		std::vector<int> magnitudes; magnitudes.reserve(LoadedSteeringSignalPercent.size());
		for (size_t index = 0; index < LoadedSteeringSignalPercent.size(); ++index)
		{
			const int magnitude = LoadedSteeringSignalPercent[index] * 100; magnitudes.push_back(magnitude);
			if (index) result.intervalsMs.push_back(double(LoadedSteeringUpdatePeriod.count()));
			if (int(index) == failAt) { ++result.failureCount; result.apiResults.push_back(-1); result.state = ResultState::Failed; break; }
			result.apiResults.push_back(0);
			if (strategy == CompatibilityStrategy::LegacyRecreation)
			{
				if (index) { ++result.stopCount; ++result.releaseCount; ++result.replacementCount; }
				++result.createCount; ++result.startCount;
			}
			else { if (index == 0) { ++result.createCount; ++result.startCount; } else ++result.updateCount; }
		}
		if (result.state == ResultState::Running)
		{
			result.state = ResultState::Completed; ++result.stopCount; ++result.releaseCount; ++result.finalCleanupCount;
		}
		finalize_compatibility_statistics(result, magnitudes); return result;
	}

	inline void finalize_compatibility_statistics(CompatibilityResult& result, const std::vector<int>& magnitudes)
	{
		if (magnitudes.empty()) return;
		double absoluteSum=0.0,squareSum=0.0;
		for(const int value:magnitudes){const int magnitude=std::abs(value);result.peakMagnitude=std::max(result.peakMagnitude,magnitude);absoluteSum+=magnitude;squareSum+=double(magnitude)*magnitude;if(value==0)result.zeroTimeMs+=CompatibilityUpdatePeriod.count();}
		result.averageMagnitude=absoluteSum/magnitudes.size();result.rmsMagnitude=std::sqrt(squareSum/magnitudes.size());
		if(!result.intervalsMs.empty())
		{
			double sum=0.0;for(const double interval:result.intervalsMs)sum+=interval;result.averageIntervalMs=sum/result.intervalsMs.size();
			double variance=0.0;for(const double interval:result.intervalsMs){const double delta=interval-result.averageIntervalMs;variance+=delta*delta;}result.jitterMs=std::sqrt(variance/result.intervalsMs.size());
		}
	}

	inline CompatibilityResult simulate_compatibility(CompatibilityStrategy strategy, int actuatorAxes=1, int failAt=-1)
	{
		CompatibilityResult result;result.strategy=strategy;result.state=ResultState::Running;result.actuatorAxes=actuatorAxes;
		std::vector<int> magnitudes; magnitudes.reserve(CompatibilitySignalPercent.size());
		for(size_t index=0;index<CompatibilitySignalPercent.size();++index)
		{
			const int magnitude=CompatibilitySignalPercent[index]*100;magnitudes.push_back(magnitude);if(index)result.intervalsMs.push_back(double(CompatibilityUpdatePeriod.count()));
			if(int(index)==failAt){++result.failureCount;result.apiResults.push_back(-1);result.state=ResultState::Failed;break;}
			result.apiResults.push_back(0);
			if(strategy==CompatibilityStrategy::LegacyRecreation)
			{
				if (index) { ++result.stopCount; ++result.releaseCount; ++result.replacementCount; }
				++result.createCount; ++result.startCount;
			}
			else {if(index==0){++result.createCount;++result.startCount;}else ++result.updateCount;}
		}
		if(result.state==ResultState::Running)
		{
			result.state=ResultState::Completed; ++result.stopCount; ++result.releaseCount; ++result.finalCleanupCount;
		}
		finalize_compatibility_statistics(result,magnitudes);return result;
	}

	inline CompatibilityClassification classify_compatibility(const CompatibilityResult& dynamic)
	{
		if(dynamic.state==ResultState::Failed||dynamic.failureCount)return CompatibilityClassification::DynamicRejected;
		if(dynamic.state!=ResultState::Completed)return CompatibilityClassification::Inconclusive;
		if(dynamic.physical==PhysicalConfirmation::No)return CompatibilityClassification::DynamicAcceptedIneffective;
		if(dynamic.physical==PhysicalConfirmation::Yes)return CompatibilityClassification::DynamicAccepted;
		return CompatibilityClassification::DynamicAcceptedUnverified;
	}

	struct Device
	{
		std::string id;
		std::string name;
		std::string backend;
		uint16_t vendor = 0;
		uint16_t product = 0;
		int axes = 0;
		int buttons = 0;
		int hats = 0;
		bool inputAvailable = false;
		bool ffbAvailable = false;
	};

	struct BackendResult
	{
		std::string name;
		std::string effectiveBackend;
		ResultState state = ResultState::Untested;
		std::string startedUtc;
		std::string error;
		std::vector<Device> devices;
		std::vector<std::string> delayedEvents;
		std::vector<std::string> openingResults;
	};

	struct ShakeController
	{
		static constexpr int FrequencyHz = 10;
		static constexpr auto HalfPeriod = std::chrono::milliseconds(1000 / (FrequencyHz * 2));
		bool active = false;
		bool right = false;
		std::chrono::steady_clock::time_point nextTransition{};

		void begin(std::chrono::steady_clock::time_point now, bool initialRight = false)
		{
			active = true; right = initialRight; nextTransition = now + HalfPeriod;
		}
		std::optional<bool> update(std::chrono::steady_clock::time_point now)
		{
			if (!active || now < nextTransition) return std::nullopt;
			right = !right;
			do nextTransition += HalfPeriod; while (nextTransition <= now);
			return right;
		}
		void stop() { active = false; }
	};

	struct Assignment
	{
		std::string steering;
		std::string pedals;
		std::string shifter;
		std::string additional;
		std::string ffb;
	};

	struct CapturedInput
	{
		std::string deviceId;
		std::string deviceName;
		std::string control;
		bool ambiguous = false;
	};

	struct QuickSetupController
	{
		static constexpr auto CaptureTime = std::chrono::seconds(6);
		int step = 0;
		bool active = false;
		bool timedOut = false;
		std::optional<CapturedInput> candidate;
		std::chrono::steady_clock::time_point deadline{};
		std::vector<std::optional<CapturedInput>> saved = std::vector<std::optional<CapturedInput>>(7);
		std::vector<std::optional<CapturedInput>> backup;

		void start(std::chrono::steady_clock::time_point now)
		{
			backup = saved; step = 0; active = true; retry(now);
		}
		void retry(std::chrono::steady_clock::time_point now)
		{
			candidate.reset(); timedOut = false; deadline = now + CaptureTime;
		}
		void observe(CapturedInput input) { if (active && !timedOut && !candidate) candidate = std::move(input); }
		bool continue_step(std::chrono::steady_clock::time_point now)
		{
			if (!active || !candidate || candidate->ambiguous) return false;
			saved[step] = candidate;
			if (++step >= int(saved.size())) { active = false; return true; }
			retry(now); return true;
		}
		void skip(std::chrono::steady_clock::time_point now)
		{
			if (!active) return;
			if (++step >= int(saved.size())) { active = false; return; }
			retry(now);
		}
		void back(std::chrono::steady_clock::time_point now)
		{
			if (!active || step == 0) return;
			--step; retry(now);
		}
		void cancel() { if (!backup.empty()) saved = backup; active = false; candidate.reset(); timedOut = false; }
		void update(std::chrono::steady_clock::time_point now) { if (active && !candidate && now >= deadline) timedOut = true; }
	};

	enum class QuickFfbResponse { NotTested, Confirmed, NotConfirmed };
	enum class QuickFfbStage { Offer, Countdown, Running, Confirm, RetryChoice, Complete };
	struct QuickFfbCheck
	{
		static constexpr auto CountdownTime=std::chrono::seconds(3);
		static constexpr auto ShakeTime=std::chrono::milliseconds(1200);
		QuickFfbStage stage=QuickFfbStage::Offer;
		QuickFfbResponse response=QuickFfbResponse::NotTested;
		std::chrono::steady_clock::time_point deadline{},started{};
		void begin(std::chrono::steady_clock::time_point now){stage=QuickFfbStage::Countdown;response=QuickFfbResponse::NotTested;deadline=now+CountdownTime;}
		bool countdown_complete(std::chrono::steady_clock::time_point now)const{return stage==QuickFfbStage::Countdown&&now>=deadline;}
		void start_output(std::chrono::steady_clock::time_point now){stage=QuickFfbStage::Running;started=now;}
		bool output_complete(std::chrono::steady_clock::time_point now)const{return stage==QuickFfbStage::Running&&now-started>=ShakeTime;}
		void answer(QuickFfbResponse value){response=value;stage=value==QuickFfbResponse::NotConfirmed?QuickFfbStage::RetryChoice:QuickFfbStage::Complete;}
		void skip(){response=QuickFfbResponse::NotTested;stage=QuickFfbStage::Complete;}
		void retry(){stage=QuickFfbStage::Offer;response=QuickFfbResponse::NotTested;}
	};

	enum class DeliveryStage { Idle, Ready, Countdown, Legacy, LegacyFeedback, SafetyInterval, Dynamic, DynamicFeedback, Shutdown, Results, Cancelled };
	enum class LoadedSteeringStage { Idle, Ready, Countdown, Legacy, LegacyFeedback, SafetyInterval, Dynamic, DynamicFeedback, Results, Cancelled };
	inline void reset_loaded_steering_session(LoadedSteeringStage& stage,CompatibilityStrategy& pending,
		CompatibilityResult& simulatedLegacy,CompatibilityResult& simulatedDynamic,
		CompatibilityResult& physicalLegacy,CompatibilityResult& physicalDynamic)
	{
		stage=LoadedSteeringStage::Idle;pending=CompatibilityStrategy::LegacyRecreation;
		simulatedLegacy={CompatibilityStrategy::LegacyRecreation};simulatedDynamic={CompatibilityStrategy::PersistentDynamic};
		physicalLegacy={CompatibilityStrategy::LegacyRecreation};physicalDynamic={CompatibilityStrategy::PersistentDynamic};
	}
	inline float delivery_progress(DeliveryStage stage,size_t signalIndex=0)
	{
		const float signal=std::clamp(float(signalIndex)/float(CompatibilitySignalPercent.size()),0.0f,1.0f);
		switch(stage){case DeliveryStage::Idle:return 0.0f;case DeliveryStage::Ready:return 1.0f/8.0f;case DeliveryStage::Countdown:return 2.0f/8.0f;case DeliveryStage::Legacy:return (2.0f+signal)/8.0f;case DeliveryStage::LegacyFeedback:return 3.0f/8.0f;case DeliveryStage::SafetyInterval:return 4.0f/8.0f;case DeliveryStage::Dynamic:return (4.0f+signal)/8.0f;case DeliveryStage::DynamicFeedback:return 6.0f/8.0f;case DeliveryStage::Shutdown:return 7.0f/8.0f;case DeliveryStage::Results:return 1.0f;case DeliveryStage::Cancelled:return 0.0f;}return 0.0f;
	}

	enum class FfbResolution { NotFound, Restored, AutoSelected, SelectionRequired };

	struct FfbResolutionResult
	{
		FfbResolution state = FfbResolution::NotFound;
		int index = -1;
	};

	inline FfbResolutionResult resolve_ffb_device(const std::vector<std::string>& identities, std::string_view previous)
	{
		if (!previous.empty())
			for (size_t index = 0; index < identities.size(); ++index)
				if (identities[index] == previous) return { FfbResolution::Restored, int(index) };
		if (identities.empty()) return {};
		if (identities.size() == 1) return { FfbResolution::AutoSelected, 0 };
		return { FfbResolution::SelectionRequired, -1 };
	}

	inline FfbResolutionResult resolve_exact_ffb_device(const std::vector<std::string>& identities, std::string_view previous)
	{
		if (previous.empty())
			return identities.empty() ? FfbResolutionResult{} : FfbResolutionResult{ FfbResolution::SelectionRequired, -1 };
		for (size_t index = 0; index < identities.size(); ++index)
			if (identities[index] == previous) return { FfbResolution::Restored, int(index) };
		return {};
	}

	enum class RedetectExecutionResult { InputRefreshFailed, NativeRefreshFailed, Ready };
	enum class AuthorizedOutputPreparation { ReacquireFailed, ActuatorEnableFailed, Ready };

	template<typename ReleaseNative, typename RefreshInput, typename RefreshNative>
	RedetectExecutionResult execute_redetect_lifecycle(ReleaseNative&& releaseNative,
		RefreshInput&& refreshInput, RefreshNative&& refreshNative)
	{
		releaseNative();
		if (!refreshInput()) return RedetectExecutionResult::InputRefreshFailed;
		if (!refreshNative()) return RedetectExecutionResult::NativeRefreshFailed;
		return RedetectExecutionResult::Ready;
	}

	template<typename Reacquire, typename EnableActuators>
	AuthorizedOutputPreparation prepare_authorized_output_lifecycle(Reacquire&& reacquire,
		EnableActuators&& enableActuators)
	{
		if (!reacquire()) return AuthorizedOutputPreparation::ReacquireFailed;
		if (!enableActuators()) return AuthorizedOutputPreparation::ActuatorEnableFailed;
		return AuthorizedOutputPreparation::Ready;
	}

	struct RedetectLifecycle
	{
		bool effectStopped = false;
		bool deviceReleased = false;
		bool inputRefreshed = false;
		bool enumerationRefreshed = false;
		bool identityResolved = false;
		bool capabilitiesValidated = false;
		bool acquired = false;
		bool zeroForce = true;
	};

	inline std::string sanitize_filename_component(std::string_view value)
	{
		std::string out;
		bool separator = false;
		for (const unsigned char c : value)
		{
			const bool invalid = c < 32 || c == '<' || c == '>' || c == ':' || c == '"' || c == '/' || c == '\\' || c == '|' || c == '?' || c == '*';
			if (invalid || c == ' ' || c == '-' || c == '.') { separator = !out.empty(); continue; }
			if (separator) { out.push_back('_'); separator = false; }
			out.push_back(char(c));
		}
		while (!out.empty() && out.back() == '_') out.pop_back();
		return out.empty() ? "No_Wheel_Detected" : out;
	}

	struct SafetyController
	{
		bool authorized = false;
		bool running = false;
		std::chrono::steady_clock::time_point started{};
		std::chrono::steady_clock::time_point heartbeat{};

		int bounded_magnitude(int requestedPercent) const
		{
			return safety_limited_magnitude(requestedPercent);
		}

		bool begin(bool selected, bool focused, std::chrono::steady_clock::time_point now)
		{
			if (!authorized || !selected || !focused) return false;
			running = true;
			started = heartbeat = now;
			return true;
		}

		void beat(std::chrono::steady_clock::time_point now) { heartbeat = now; }
		bool must_stop(bool held, bool focused, bool connected, std::chrono::steady_clock::time_point now) const
		{
			return running && (!held || !focused || !connected || now - started >= MaximumRunTime || now - heartbeat >= WatchdogTimeout);
		}
		void stop() { running = false; }
	};
}
