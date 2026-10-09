#include "surface_renderer.hpp"
#include <cassert>
#include <cmath>
#include <string_view>

int main()
{
	using namespace HYP36RSurfaceRenderer;
	static_assert(DefaultStrengthPercent == 100);
	static_assert(MaximumStrengthPercent == 100);
	static_assert(DefaultAmplitudeCeilingPercent == 18);
	static_assert(NormalAmplitudeCeilingPercent == 18);
	static_assert(MaximumAmplitudeCeilingPercent == 25);
	static_assert(DefaultPlayerSurfacePercent == 50);
	static_assert(DefaultWaveform == Waveform::Triangle);
	static_assert(Input{}.waveform == Waveform::Triangle);
	static_assert(Request{}.waveform == Waveform::Triangle);
	assert(resolve_amplitude_ceiling_percent(false, 12) == 18);
	assert(resolve_amplitude_ceiling_percent(false, 25) == 18);
	for (int ceiling : AmplitudeCeilingPercents)
		assert(resolve_amplitude_ceiling_percent(true, ceiling) == ceiling);
	assert(resolve_amplitude_ceiling_percent(true, 99) == 18);
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
	independentSurface.strengthPercent = 100; independentSurface.amplitudeCeilingPercent = 25;
	assert(evaluate(independentSurface).boundedMagnitude == 0.25f);
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
	assert(waveform_from_string("sine") == Waveform::Sine);
	assert(waveform_from_string("invalid") == Waveform::Triangle);
	assert(std::string_view(waveform_name(Waveform::Sine)) == "Sine");
	assert(std::string_view(waveform_name(Waveform::Triangle)) == "Triangle");
	assert(std::string_view(waveform_name(Waveform::Square)) == "Square");
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
	static_assert(AmplitudeCeilingPercents == std::array{ 12, 18, 25 });
	assert(sanitize_player_surface_percent(-1) == 0);
	assert(sanitize_player_surface_percent(50) == 50);
	assert(sanitize_player_surface_percent(101) == 100);
	assert(std::abs(player_texture_scale(0) - 0.0f) < 0.000001f);
	assert(std::abs(player_texture_scale(50) - 0.5f) < 0.000001f);
	assert(std::abs(player_texture_scale(100) - 1.0f) < 0.000001f);
	assert(std::abs(player_bump_strength_percent(0) - 0.0f) < 0.000001f);
	assert(std::abs(player_bump_strength_percent(50) - 15.0f) < 0.000001f);
	assert(std::abs(player_bump_strength_percent(100) - 30.0f) < 0.000001f);
	static_assert(BumpThreshold == 0.020f && BumpStrengthPercent == 30);
	static_assert(BumpMaximumMagnitude == 0.30f);
	static_assert(BumpDurationMilliseconds == 60 && BumpCooldownMilliseconds == 120);
	assert(std::abs(bound_bump_transport_magnitude(0.22f) - 0.22f) < 0.000001f);
	assert(std::abs(bound_bump_transport_magnitude(0.30f) - 0.30f) < 0.000001f);
	assert(bound_bump_transport_magnitude(0.30f) > 0.25f);
	assert(std::abs(bound_bump_transport_magnitude(0.45f) - 0.30f) < 0.000001f);
	assert(std::abs(bound_bump_transport_magnitude(-0.45f) + 0.30f) < 0.000001f);
	assert(bound_bump_transport_magnitude(NAN) == 0.0f);
	// The deprecated persisted A/B value is accepted but no longer authoritative.
	assert(resolve_bump_enabled(50, false));
	assert(resolve_bump_enabled(50, true));
	assert(!resolve_bump_enabled(0, false));
	assert(!resolve_bump_enabled(0, true));

	// Road Mode is deliberately absent from the Surface request and Bump APIs.
	// Equivalent shared input therefore produces identical Texture and Bump output.
	const Input parityInput{ Renderer::Surface, 0.40f, 0.5f, 50, 18,
		Waveform::Sine, FrequencyProfile::Reference, true, true, true };
	const auto classicTexture = evaluate(parityInput);
	const auto enhancedTexture = evaluate(parityInput);
	assert(classicTexture.sourceRoad == enhancedTexture.sourceRoad);
	assert(classicTexture.requestedMagnitude == enhancedTexture.requestedMagnitude);
	assert(classicTexture.boundedMagnitude == enhancedTexture.boundedMagnitude);
	BumpDetector classicBump;
	BumpDetector enhancedBump;
	for (float source : { 0.0f, 0.01f, 0.05f })
	{
		const BumpInput input{ source, 1.0f / 60.0f, BumpThreshold,
			player_bump_strength_percent(50), BumpDurationMilliseconds,
			BumpCooldownMilliseconds, true };
		const auto classicFrame = classicBump.evaluate(input);
		const auto enhancedFrame = enhancedBump.evaluate(input);
		assert(classicFrame.transientMetric == enhancedFrame.transientMetric);
		assert(classicFrame.candidate == enhancedFrame.candidate);
		assert(classicFrame.triggered == enhancedFrame.triggered);
		assert(classicFrame.boundedMagnitude == enhancedFrame.boundedMagnitude);
	}

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
	assert(boundedBump.triggered && boundedBump.boundedMagnitude == 0.30f);
	bump.reset(); bumpInput.enabled = false; bumpInput.surfaceSource = 1.0f;
	assert(!bump.evaluate(bumpInput).triggered);

	// Release behavior: a stale persisted Bump Off cannot suppress a qualifying
	// transient while player Surface is active, and Surface 0 cannot trigger.
	BumpDetector automaticBump;
	BumpInput automaticInput{ 0.0f, 1.0f / 60.0f, BumpThreshold,
		player_bump_strength_percent(100), BumpDurationMilliseconds,
		BumpCooldownMilliseconds, resolve_bump_enabled(100, false) };
	assert(!automaticBump.evaluate(automaticInput).triggered);
	automaticInput.surfaceSource = 1.0f;
	assert(automaticBump.evaluate(automaticInput).triggered);
	BumpDetector disabledBump;
	automaticInput.enabled = resolve_bump_enabled(0, true);
	automaticInput.surfaceSource = 0.0f;
	assert(!disabledBump.evaluate(automaticInput).triggered);
	automaticInput.surfaceSource = 1.0f;
	assert(!disabledBump.evaluate(automaticInput).triggered);
}
