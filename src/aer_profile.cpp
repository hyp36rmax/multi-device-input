#include "aer_profile.hpp"

#include <algorithm>
#include <cmath>

namespace HYP36RAer
{
	namespace
	{
		float finite(float value) noexcept { return std::isfinite(value) ? value : 0.0f; }
		float clamp_unit(float value) noexcept { return (std::clamp)(finite(value), -1.0f, 1.0f); }
		float approach(float current, float target, float step) noexcept
		{
			return current + (std::clamp)(target - current, -step, step);
		}
	}

	Profile profile_from_int(int value) noexcept
	{
		return value == 1 ? Profile::ArcadeExperienceExperimental : Profile::ReferencePlus;
	}

	const char* profile_name(Profile profile) noexcept
	{
		return profile == Profile::ArcadeExperienceExperimental
			? "Arcade Experience (Experimental)" : "Reference+";
	}

	AerEvidenceFrame adapt_evidence(const HYP36RSignalState::Frame& source) noexcept
	{
		AerEvidenceFrame out{};
		out.frameId = source.frameId;
		out.valid = source.vehicle.meta.validity == HYP36RSignalState::Validity::Valid;
		out.steering = finite(source.vehicle.steering);
		out.normalizedSpeed = (std::clamp)(finite(source.vehicle.normalizedSpeed), 0.0f, 1.0f);
		out.responseAngle = finite(source.vehicle.responseAngle);
		out.responseAuthority = (std::clamp)(finite(source.vehicle.responseAuthority), 0.0f, 1.0f);
		out.roadActivity = (std::clamp)(finite(source.roadIntent.continuousActivityEvidence), 0.0f, 1.0f);
		out.impactEvidence = (std::clamp)(finite((std::max)(source.eventIntent.riseEvidence,
			source.eventIntent.existingImpactEvidence)), 0.0f, 1.0f);
		out.surfaces = source.surfaces.current;
		out.surfaceChanged = source.surfaces.changed;
		out.mixedContact = source.occupancy.mixed;
		return out;
	}

	ContinuousOutput AerContinuousInterpreter::evaluate(const AerEvidenceFrame& e, float dt) noexcept
	{
		if (!e.valid || !std::isfinite(dt) || dt <= 0.0f)
		{
			filtered_ = approach(filtered_, 0.0f, 4.0f * (std::max)(finite(dt), 0.0f));
			return { filtered_ };
		}
		const float directionEvidence = clamp_unit(e.responseAngle * 1.8f + e.steering * 0.35f);
		const float magnitude = std::pow(std::abs(directionEvidence), 0.72f);
		const float speedAuthority = 0.22f + 0.78f * e.normalizedSpeed;
		const float contactAttenuation = e.mixedContact ? 0.80f : 1.0f;
		const float responseAuthority = 0.45f + 0.55f * e.responseAuthority;
		const float target = std::copysign(magnitude * speedAuthority * contactAttenuation *
			responseAuthority * 0.78f, directionEvidence);
		filtered_ = approach(filtered_, clamp_unit(target), (std::clamp)(dt, 0.0f, 0.05f) * 5.0f);
		return { filtered_ };
	}

	void AerEventInterpreter::reset() noexcept
	{
		current_ = {};
		cooldownSeconds_ = 0.0f;
	}

	EventOutput AerEventInterpreter::evaluate(const AerEvidenceFrame& e, int roadDetailPercent, float dt) noexcept
	{
		dt = (std::clamp)(finite(dt), 0.0f, 0.05f);
		cooldownSeconds_ = (std::max)(0.0f, cooldownSeconds_ - dt);
		current_.remainingSeconds = (std::max)(0.0f, current_.remainingSeconds - dt);
		if (current_.remainingSeconds <= 0.0f)
			current_ = {};
		if (!e.valid || cooldownSeconds_ > 0.0f)
			return current_;

		const float roadScale = float((std::clamp)(roadDetailPercent, 0, 100)) / 100.0f;
		EventOutput next{};
		if (e.impactEvidence >= 0.10f)
		{
			next = { EventClass::Impact, std::copysign((std::min)(0.38f, 0.16f + e.impactEvidence * 0.28f),
				std::abs(e.steering) > 0.03f ? e.steering : 1.0f), 0.09f, true };
		}
		else
		{
			const bool transition = std::any_of(e.surfaceChanged.begin(), e.surfaceChanged.end(), [](bool v) { return v; });
			if (transition)
				next = { e.mixedContact ? EventClass::ContactAsymmetry : EventClass::SurfaceTransition,
					std::copysign((e.mixedContact ? 0.18f : 0.12f) * roadScale,
					std::abs(e.steering) > 0.03f ? e.steering : 1.0f), 0.07f, true };
		}
		if (next.active)
		{
			current_ = next;
			cooldownSeconds_ = next.classification == EventClass::Impact ? 0.12f : 0.06f;
		}
		return current_;
	}

	float AerComposer::compose(float continuous, float event, float dt, bool& limited) noexcept
	{
		const float preLimit = finite(continuous) + finite(event);
		const float soft = std::tanh(preLimit);
		const float target = (std::clamp)(soft, -0.92f, 0.92f);
		limited = std::abs(target - preLimit) > 0.0001f;
		const float maxStep = (std::clamp)(finite(dt), 0.0f, 0.05f) * 7.0f;
		previous_ = approach(previous_, target, maxStep);
		return previous_;
	}

	float AerSafetyGate::apply(float request, const AerEvidenceFrame& evidence,
		const SafetyInputs& safety, float dt, bool& open) noexcept
	{
		open = safety.profileSelected && safety.inGameplay && safety.ffbEnabled &&
			safety.deviceReady && safety.focused;
		dt = (std::clamp)(finite(dt), 0.0f, 0.05f);
		if (evidence.valid) staleSeconds_ = 0.0f; else staleSeconds_ += dt;
		if (!open || staleSeconds_ > 0.10f || !std::isfinite(request))
		{
			held_ = approach(held_, 0.0f, dt * 8.0f);
			return held_;
		}
		held_ = clamp_unit(request);
		return held_;
	}

	const Frame& Runtime::evaluate(const AerEvidenceFrame& evidence, const AerProfileSettings& settings,
		const SafetyInputs& safety, float dt) noexcept
	{
		frame_.evidence = evidence;
		frame_.continuous = continuous_.evaluate(evidence, dt);
		frame_.event = events_.evaluate(evidence, settings.roadDetailPercent, dt);
		bool limited = false;
		const float strength = float((std::clamp)(settings.strengthPercent, 0, 100)) / 100.0f;
		const float preLimit = frame_.continuous.requested + frame_.event.requested;
		const float composed = composer_.compose(frame_.continuous.requested,
			frame_.event.requested, dt, limited) * strength;
		bool safetyOpen = false;
		const float safe = safety_.apply(composed, evidence, safety, dt, safetyOpen);
		frame_.telemetry = { TelemetrySchemaVersion, safety.profileSelected,
			settings.mode == OperatingMode::Active && safetyOpen, evidence.valid,
			frame_.continuous.requested, frame_.event.classification, frame_.event.requested,
			preLimit, safe,
			limited, safetyOpen, (std::clamp)(settings.strengthPercent, 0, 100),
			(std::clamp)(settings.roadDetailPercent, 0, 100) };
		return frame_;
	}

	void Runtime::reset() noexcept
	{
		continuous_.reset(); events_.reset(); composer_.reset(); safety_.reset(); frame_ = {};
	}

	const char* event_name(EventClass eventClass) noexcept
	{
		switch (eventClass)
		{
		case EventClass::SurfaceTransition: return "surface_transition";
		case EventClass::ContactAsymmetry: return "contact_asymmetry";
		case EventClass::Impact: return "impact";
		default: return "none";
		}
	}
}
