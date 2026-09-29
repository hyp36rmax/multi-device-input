#include "surface_renderer.hpp"
#include <cassert>
#include <cmath>

int main()
{
	using namespace HYP36RSurfaceRenderer;
	static_assert(DefaultStrengthPercent == 100);
	static_assert(MaximumStrengthPercent == 100);
	static_assert(DefaultAmplitudeCeilingPercent == 12);
	const Input base{ Renderer::Directional, 0.2f, 0.5f, 25, 12, Waveform::Sine, FrequencyProfile::Reference, true, true, true };
	const auto directional = evaluate(base);
	assert(directional.directionalRoad == 0.2f && !directional.active);
	assert(directional.boundedMagnitude == 0.0f);

	auto surfaceInput = base; surfaceInput.renderer = Renderer::Surface;
	const auto surface = evaluate(surfaceInput);
	assert(surface.directionalRoad == 0.0f);
	assert(std::abs(surface.requestedMagnitude - 0.05f) < 0.000001f);
	assert(surface.boundedMagnitude == surface.requestedMagnitude && surface.active);
	assert(surface.frequencyHz == 30.0f);
	const float steering = -0.3f, impact = 0.08f;
	assert(steering + impact + directional.directionalRoad == steering + impact + 0.2f);
	assert(steering + impact + surface.directionalRoad == steering + impact);
	// The caller can preserve the conditioned Directional Road while giving
	// Surface the shared pre-directional-conditioning source.
	auto independentSurface = surfaceInput; independentSurface.directionalRoad = 0.80f;
	independentSurface.strengthPercent = 100; independentSurface.amplitudeCeilingPercent = 50;
	assert(evaluate(independentSurface).boundedMagnitude == 0.50f);
	assert(directional.directionalRoad == 0.20f);

	surfaceInput.directionalRoad = 1.0f; surfaceInput.strengthPercent = 100;
	const auto bounded = evaluate(surfaceInput);
	assert(bounded.boundedMagnitude == 0.12f && bounded.boundActive);
	surfaceInput.periodicSupported = false; assert(!evaluate(surfaceInput).active);
	surfaceInput.periodicSupported = true; surfaceInput.inGameplay = false; assert(!evaluate(surfaceInput).active);
	surfaceInput.inGameplay = true; surfaceInput.ffbEnabled = false; assert(!evaluate(surfaceInput).active);
	surfaceInput.ffbEnabled = true; surfaceInput.normalizedSpeed = -1.0f;
	assert(evaluate(surfaceInput).frequencyHz == MinimumFrequencyHz);
	surfaceInput.normalizedSpeed = 2.0f;
	assert(evaluate(surfaceInput).frequencyHz == MaximumFrequencyHz);
	for (int ceiling : AmplitudeCeilingPercents)
	{
		surfaceInput.directionalRoad = 1.0f;
		surfaceInput.strengthPercent = 100;
		surfaceInput.amplitudeCeilingPercent = ceiling;
		const auto selected = evaluate(surfaceInput);
		assert(selected.amplitudeCeilingPercent == ceiling);
		assert(std::abs(selected.boundedMagnitude - float(ceiling) / 100.0f) < 0.000001f);
		assert(selected.requestedMagnitude >= selected.boundedMagnitude);
	}
	surfaceInput.amplitudeCeilingPercent = 49;
	assert(evaluate(surfaceInput).amplitudeCeilingPercent == DefaultAmplitudeCeilingPercent);
	assert(renderer_from_string("surface") == Renderer::Surface);
	assert(renderer_from_string("invalid") == Renderer::Directional);
	assert(waveform_from_string("triangle") == Waveform::Triangle);
	assert(waveform_from_string("square") == Waveform::Square);
	assert(waveform_from_string("invalid") == Waveform::Sine);
	for (const auto profile : { FrequencyProfile::Low, FrequencyProfile::Reference, FrequencyProfile::Medium, FrequencyProfile::High })
	{
		surfaceInput.frequencyProfile = profile; surfaceInput.normalizedSpeed = 0.0f;
		assert(evaluate(surfaceInput).frequencyHz == frequency_range(profile).minimumHz);
		surfaceInput.normalizedSpeed = 1.0f;
		assert(evaluate(surfaceInput).frequencyHz == frequency_range(profile).maximumHz);
	}
	assert(frequency_range(FrequencyProfile::Low).minimumHz == 12.0f && frequency_range(FrequencyProfile::Low).maximumHz == 30.0f);
	assert(frequency_range(FrequencyProfile::Reference).minimumHz == 18.0f && frequency_range(FrequencyProfile::Reference).maximumHz == 42.0f);
	assert(frequency_range(FrequencyProfile::Medium).minimumHz == 24.0f && frequency_range(FrequencyProfile::Medium).maximumHz == 48.0f);
	assert(frequency_range(FrequencyProfile::High).minimumHz == 30.0f && frequency_range(FrequencyProfile::High).maximumHz == 60.0f);
	static_assert(BoundaryResearchCeilingPercents == std::array{ 25, 50, 60, 70, 80, 90, 100 });

	BumpDetector bump;
	BumpInput bumpInput{ 0.0f, 1.0f / 60.0f, 0.02f, 100, 60, 120, false };
	assert(!bump.evaluate(bumpInput).triggered);
	bumpInput.enabled = true;
	assert(!bump.evaluate(bumpInput).triggered);
	// A qualifying transient produces one bounded, finite pulse.
	bumpInput.surfaceSource = 0.10f;
	const auto firstBump = bump.evaluate(bumpInput);
	assert(firstBump.candidate && firstBump.triggered);
	assert(std::abs(firstBump.boundedMagnitude - 0.10f) < 0.000001f);
	assert(firstBump.durationMilliseconds == 60);
	// Sustained magnitude and cooldown prevent machine-gun retriggering.
	assert(!bump.evaluate(bumpInput).triggered);
	bumpInput.surfaceSource = 0.20f;
	assert(bump.evaluate(bumpInput).candidate && !bump.frame().triggered);
	for (int i = 0; i < 8; ++i) bump.evaluate(bumpInput);
	bumpInput.surfaceSource = -1.0f; bumpInput.strengthPercent = 100;
	const auto boundedBump = bump.evaluate(bumpInput);
	assert(boundedBump.triggered && boundedBump.boundedMagnitude == 0.25f);
	bump.reset(); bumpInput.enabled = false; bumpInput.surfaceSource = 1.0f;
	assert(!bump.evaluate(bumpInput).triggered);
}
