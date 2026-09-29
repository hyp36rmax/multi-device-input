#include "surface_renderer.hpp"
#include <cassert>
#include <cmath>

int main()
{
	using namespace HYP36RSurfaceRenderer;
	const Input base{ Renderer::Directional, 0.2f, 0.5f, 25, true, true, true };
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

	surfaceInput.directionalRoad = 1.0f; surfaceInput.strengthPercent = 50;
	const auto bounded = evaluate(surfaceInput);
	assert(bounded.boundedMagnitude == MaximumMagnitude && bounded.boundActive);
	surfaceInput.periodicSupported = false; assert(!evaluate(surfaceInput).active);
	surfaceInput.periodicSupported = true; surfaceInput.inGameplay = false; assert(!evaluate(surfaceInput).active);
	surfaceInput.inGameplay = true; surfaceInput.ffbEnabled = false; assert(!evaluate(surfaceInput).active);
	surfaceInput.ffbEnabled = true; surfaceInput.normalizedSpeed = -1.0f;
	assert(evaluate(surfaceInput).frequencyHz == MinimumFrequencyHz);
	surfaceInput.normalizedSpeed = 2.0f;
	assert(evaluate(surfaceInput).frequencyHz == MaximumFrequencyHz);
	assert(renderer_from_string("surface") == Renderer::Surface);
	assert(renderer_from_string("invalid") == Renderer::Directional);
}
