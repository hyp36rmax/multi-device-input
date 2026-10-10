#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <cmath>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace DeviceDiagnostics
{
	inline constexpr const char* Version = "0.1.0-dev";
	inline constexpr int PhysicalOutputCeilingPercent = 20;
	inline constexpr auto MaximumRunTime = std::chrono::milliseconds(1500);
	inline constexpr auto WatchdogTimeout = std::chrono::milliseconds(250);
	inline constexpr auto BackendObservationTime = std::chrono::seconds(10);
	inline constexpr auto CompatibilityUpdatePeriod = std::chrono::milliseconds(66);
	inline constexpr std::array<int, 12> CompatibilitySignalPercent{ 0, 5, 10, 15, 20, 15, 10, 0, -10, -20, -10, 0 };
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
	enum class CompatibilityClassification { Inconclusive, DynamicAccepted, DynamicRejected, DynamicAcceptedUnverified, DynamicAcceptedIneffective };

	struct CompatibilityResult
	{
		CompatibilityStrategy strategy = CompatibilityStrategy::LegacyRecreation;
		ResultState state = ResultState::Untested;
		bool simulated = true;
		int actuatorAxes = 0;
		int requestedRateHz = 15;
		int createCount = 0, startCount = 0, stopCount = 0, updateCount = 0, failureCount = 0;
		int peakMagnitude = 0;
		double averageMagnitude = 0.0, rmsMagnitude = 0.0, averageIntervalMs = 0.0, jitterMs = 0.0, zeroTimeMs = 0.0;
		std::vector<long> apiResults;
		std::vector<double> intervalsMs;
		PhysicalConfirmation physical = PhysicalConfirmation::Untested;
		std::string startedUtc, completedUtc, note;
	};

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
			if(strategy==CompatibilityStrategy::LegacyRecreation){++result.createCount;++result.startCount;if(index)++result.stopCount;}
			else {if(index==0){++result.createCount;++result.startCount;}else ++result.updateCount;}
		}
		if(result.state==ResultState::Running){result.state=ResultState::Completed;++result.stopCount;}
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

	enum class DeliveryStage { Idle, Ready, Countdown, Legacy, SafetyInterval, Dynamic, Shutdown, Results, Cancelled };
	inline float delivery_progress(DeliveryStage stage,size_t signalIndex=0)
	{
		const float signal=std::clamp(float(signalIndex)/float(CompatibilitySignalPercent.size()),0.0f,1.0f);
		switch(stage){case DeliveryStage::Idle:return 0.0f;case DeliveryStage::Ready:return 2.0f/8.0f;case DeliveryStage::Countdown:return 3.0f/8.0f;case DeliveryStage::Legacy:return (3.0f+signal)/8.0f;case DeliveryStage::SafetyInterval:return 5.0f/8.0f;case DeliveryStage::Dynamic:return (5.0f+signal)/8.0f;case DeliveryStage::Shutdown:return 7.0f/8.0f;case DeliveryStage::Results:return 1.0f;case DeliveryStage::Cancelled:return 0.0f;}return 0.0f;
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

	struct RedetectLifecycle
	{
		bool effectStopped = false;
		bool deviceReleased = false;
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
