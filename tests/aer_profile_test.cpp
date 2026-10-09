#include "aer_profile.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>

using namespace HYP36RAer;

static AerEvidenceFrame valid_evidence()
{
	AerEvidenceFrame e{};
	e.frameId = 1;
	e.valid = true;
	e.steering = 0.5f;
	e.normalizedSpeed = 0.7f;
	e.responseAngle = 0.35f;
	e.responseAuthority = 0.8f;
	return e;
}

int main()
{
	assert(profile_from_int(0) == Profile::ReferencePlus);
	assert(profile_from_int(99) == Profile::ReferencePlus);
	assert(profile_from_int(1) == Profile::ArcadeExperienceExperimental);

	Runtime runtime;
	AerProfileSettings settings{};
	SafetyInputs safe{ true, true, true, true, true };
	auto e = valid_evidence();
	const auto first = runtime.evaluate(e, settings, safe, 1.0f / 60.0f);
	assert(first.telemetry.selected && first.telemetry.activeOutput);
	assert(first.continuous.requested > 0.0f);
	assert(std::isfinite(first.telemetry.finalRequest));
	assert(std::abs(first.telemetry.finalRequest) <= 1.0f);

	// Nonlinear shaping is monotonic over the useful response range.
	float previous = 0.0f;
	for (float response : { 0.05f, 0.15f, 0.30f, 0.50f })
	{
		runtime.reset();
		e.responseAngle = response;
		const auto& frame = runtime.evaluate(e, settings, safe, 0.05f);
		assert(frame.continuous.requested >= previous);
		previous = frame.continuous.requested;
	}

	// Surface changes generate bounded, Road Detail-scaled short events.
	runtime.reset();
	e = valid_evidence(); e.surfaceChanged[0] = true; e.mixedContact = true;
	const auto& surface = runtime.evaluate(e, settings, safe, 0.016f);
	assert(surface.event.classification == EventClass::ContactAsymmetry);
	assert(surface.event.active && std::abs(surface.event.requested) <= 0.18f);

	// Collision evidence has priority and remains a short bounded request.
	runtime.reset();
	e.impactEvidence = 0.8f;
	const auto& impact = runtime.evaluate(e, settings, safe, 0.016f);
	assert(impact.event.classification == EventClass::Impact);
	assert(std::abs(impact.event.requested) <= 0.38f);

	// Shadow mode calculates the same proposal but never marks output active.
	runtime.reset(); settings.mode = OperatingMode::Shadow;
	const auto& shadow = runtime.evaluate(valid_evidence(), settings, safe, 0.016f);
	assert(!shadow.telemetry.activeOutput);
	assert(shadow.telemetry.finalRequest != 0.0f);

	// Default-off/profile independence and all safety gates decay toward zero.
	runtime.reset(); settings.mode = OperatingMode::Active;
	safe.profileSelected = false;
	const auto& off = runtime.evaluate(valid_evidence(), settings, safe, 0.016f);
	assert(!off.telemetry.activeOutput && off.telemetry.finalRequest == 0.0f);
	safe.profileSelected = true; safe.deviceReady = false;
	const auto& disconnected = runtime.evaluate(valid_evidence(), settings, safe, 0.016f);
	assert(!disconnected.telemetry.safetyOpen && disconnected.telemetry.finalRequest == 0.0f);

	// Invalid/stale evidence cannot hold force indefinitely.
	runtime.reset(); safe.deviceReady = true;
	e = valid_evidence();
	runtime.evaluate(e, settings, safe, 0.016f);
	e.valid = false;
	float last = 1.0f;
	for (int i = 0; i < 90; ++i)
	{
		last = runtime.evaluate(e, settings, safe, 0.016f).telemetry.finalRequest;
		assert(std::isfinite(last));
	}
	assert(std::abs(last) < 0.0001f);

	// Invalid values are sanitized.
	e = valid_evidence(); e.responseAngle = NAN; e.responseAuthority = INFINITY;
	const auto& invalid = runtime.evaluate(e, settings, safe, 0.016f);
	assert(std::isfinite(invalid.telemetry.finalRequest));
}
