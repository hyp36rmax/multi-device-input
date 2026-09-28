#include "force_character_presentation.hpp"
#include "road2_active.hpp"
#include "overlay/road_detail_mode_ui.hpp"

#include <cassert>
#include <cmath>
#include <limits>
#include <vector>

namespace Active = HYP36RRoad2Active;
namespace Policy = HYP36RRoad2;
namespace Presentation = HYP36RRoad2Presentation;

namespace
{
	Policy::Frame authorized_policy(uint64_t frameId = 1)
	{
		Policy::Frame value{};
		value.frameId = frameId;
		value.meta.validity = HYP36RSignalState::Validity::Valid;
		value.activity.state = Policy::ActivityState::ContinuousCandidate;
		value.activity.leftEvidence = 0.8f;
		value.activity.rightEvidence = 0.7f;
		value.spatial.referenceEstablished = true;
		value.spatial.referenceRaw = 2;
		value.spatial.differingFromReference = 4;
		value.spatial.coverage = Policy::Coverage::Broad;
		value.spatial.group02.raw = { 0x100000u, 0x100000u };
		value.spatial.group13.raw = { 0x100000u, 0x100000u };
		return value;
	}

	Presentation::Frame authorized_presentation(uint64_t frameId = 1)
	{
		Presentation::Frame value{};
		value.frameId = frameId;
		value.meta.validity = HYP36RSignalState::Validity::Valid;
		value.continuous.active = true;
		value.continuous.normalized = 0.75f;
		return value;
	}

	std::vector<float> sequence(Active::Generator& generator, int count)
	{
		std::vector<float> values;
		values.reserve(count);
		for (int n = 0; n < count; ++n)
		{
			auto policy = authorized_policy(static_cast<uint64_t>(n + 1));
			auto presentation = authorized_presentation(static_cast<uint64_t>(n + 1));
			values.push_back(generator.evaluate(policy, presentation, 1.0f / 60.0f,
				Active::Mode::Experimental).contribution);
		}
		return values;
	}
}

int main()
{
	Active::Generator first;
	Active::Generator second;
	const auto a = sequence(first, 12000);
	const auto b = sequence(second, 12000);
	assert(a == b); // deterministic seed and fixed-step evolution

	float sum = 0.0f;
	for (size_t n = 600; n < a.size(); ++n)
	{
		sum += a[n];
		assert(std::isfinite(a[n]));
		assert(std::abs(a[n]) <= Active::InternalCeiling + 1.0e-6f);
		if (n > 600)
			assert(std::abs(a[n] - a[n - 1]) <=
				Active::MaximumSlewPerSecond / 60.0f + 1.0e-6f);
	}
	assert(std::abs(sum / static_cast<float>(a.size() - 600)) < 0.0015f);

	first.reset();
	assert(sequence(first, 300) == std::vector<float>(a.begin(), a.begin() + 300));

	// Fixed 120 Hz generator evolution is stable when the same duration is
	// presented through 60 Hz or 120 Hz update cadence.
	Active::Generator sixtyHz;
	Active::Generator oneTwentyHz;
	for (int n = 0; n < 600; ++n)
	{
		const auto at60 = sixtyHz.evaluate(authorized_policy(n + 1),
			authorized_presentation(n + 1), 1.0f / 60.0f, Active::Mode::Experimental);
		oneTwentyHz.evaluate(authorized_policy(n * 2 + 1),
			authorized_presentation(n * 2 + 1), 1.0f / 120.0f, Active::Mode::Experimental);
		const auto at120 = oneTwentyHz.evaluate(authorized_policy(n * 2 + 2),
			authorized_presentation(n * 2 + 2), 1.0f / 120.0f, Active::Mode::Experimental);
		assert(std::abs(at60.contribution - at120.contribution) < 1.0e-6f);
	}

	auto policy = authorized_policy();
	auto presentation = authorized_presentation();
	presentation.continuous.normalized = 0.0f;
	presentation.continuous.active = false;
	const auto zero = second.evaluate(policy, presentation, 1.0f / 60.0f,
		Active::Mode::Experimental);
	assert(zero.contribution == 0.0f);
	assert(zero.safety == Active::SafetyState::AuthorityZero);
	assert(zero.resistanceComponent == 0.0f);

	presentation = authorized_presentation();
	presentation.continuous.normalized = std::numeric_limits<float>::quiet_NaN();
	const auto invalid = second.evaluate(policy, presentation, 1.0f / 60.0f,
		Active::Mode::Experimental);
	assert(invalid.contribution == 0.0f);
	assert(invalid.phase == Active::Phase::FailSafe);
	assert(invalid.reset);

	Active::Generator transition;
	auto partial = authorized_policy();
	partial.spatial.differingFromReference = 1;
	partial.spatial.coverage = Policy::Coverage::Partial;
	const auto attack = transition.evaluate(partial, authorized_presentation(), 1.0f / 60.0f,
		Active::Mode::Experimental);
	assert(attack.phase == Active::Phase::Attack);
	assert(attack.occupancyEnvelope > 0.0f && attack.occupancyEnvelope <= 0.25f);
	policy = authorized_policy();
	const auto full = transition.evaluate(policy, authorized_presentation(), 1.0f / 60.0f,
		Active::Mode::Experimental);
	assert(full.phase == Active::Phase::Attack);
	Presentation::Frame silent{};
	silent.continuous.normalized = 0.0f;
	silent.continuous.active = false;
	const auto release = transition.evaluate(policy, silent, 1.0f / 60.0f,
		Active::Mode::Experimental);
	assert(release.phase == Active::Phase::Release);
	assert(release.contribution == 0.0f); // authority gate always wins over release shaping

	// Only controlled, documented native surface classes receive named
	// presentation character. Grass and sand remain one conservative class.
	policy = authorized_policy();
	assert(Active::classify_surface(policy) == Active::SurfaceArchetype::HardUneven);
	policy.spatial.group02.raw = { 4u, 8u };
	policy.spatial.group13.raw = { 0x2000u, 4u };
	assert(Active::classify_surface(policy) == Active::SurfaceArchetype::SoftRough);
	policy.spatial.group02.raw = { 0x400u, 0x800u };
	policy.spatial.group13.raw = { 0x400u, 0x800u };
	assert(Active::classify_surface(policy) == Active::SurfaceArchetype::StripedRunoff);
	policy.spatial.group02.raw = { 0x20u, 0x20u };
	policy.spatial.group13.raw = { 0x20u, 0x20u };
	assert(Active::classify_surface(policy) == Active::SurfaceArchetype::GenericEnhanced);
	policy.spatial.differingFromReference = 0;
	assert(Active::classify_surface(policy) == Active::SurfaceArchetype::None);

	auto character_is_observable = [](Policy::Frame surfacePolicy,
		Active::SurfaceArchetype expected, bool expectCharacter)
	{
		Active::Generator generator;
		bool observedBase = false;
		bool observedCharacter = false;
		for (uint64_t frame = 1; frame <= 600; ++frame)
		{
			surfacePolicy.frameId = frame;
			const auto output = generator.evaluate(surfacePolicy, authorized_presentation(frame),
				1.0f / 60.0f, Active::Mode::Experimental);
			assert(output.archetype == expected);
			observedBase |= std::abs(output.aperiodicBase) > 1.0e-8f;
			observedCharacter |= std::abs(output.characterComponent) > 1.0e-8f;
			assert(output.resistanceComponent == 0.0f);
		}
		assert(observedBase);
		assert(observedCharacter == expectCharacter);
	};
	policy = authorized_policy();
	character_is_observable(policy, Active::SurfaceArchetype::HardUneven, true);
	policy.spatial.group02.raw = { 4u, 8u };
	policy.spatial.group13.raw = { 0x2000u, 4u };
	character_is_observable(policy, Active::SurfaceArchetype::SoftRough, true);
	policy.spatial.group02.raw = { 0x400u, 0x800u };
	policy.spatial.group13.raw = { 0x400u, 0x800u };
	character_is_observable(policy, Active::SurfaceArchetype::StripedRunoff, true);
	policy.spatial.group02.raw = { 0x20u, 0x20u };
	policy.spatial.group13.raw = { 0x20u, 0x20u };
	character_is_observable(policy, Active::SurfaceArchetype::GenericEnhanced, false);

	Active::Generator normalRoad;
	policy = authorized_policy();
	policy.spatial.differingFromReference = 0;
	policy.spatial.coverage = Policy::Coverage::Reference;
	for (uint64_t frame = 1; frame <= 120; ++frame)
	{
		policy.frameId = frame;
		assert(normalRoad.evaluate(policy, authorized_presentation(frame), 1.0f / 60.0f,
			Active::Mode::Experimental).contribution == 0.0f);
	}

	// Reference+ selection is exact and cannot read the experimental channel.
	assert(Active::select_road(Active::Mode::ReferencePlus, 0.1234567f, -0.05f) == 0.1234567f);
	assert(Active::select_road(Active::Mode::Experimental, 0.1234567f, -0.05f) == -0.05f);

	const HYP36RForceCharacter::Channels source{ 0.31f, 0.04f, -0.22f };
	const auto baseline = HYP36RForceCharacter::apply(source, { 100, 100, 100 });
	const auto roadZero = HYP36RForceCharacter::apply(source, { 100, 0, 100 });
	const auto roadMaximum = HYP36RForceCharacter::apply(source, { 100, 200, 100 });
	assert(roadZero.road == 0.0f && roadMaximum.road == 0.08f);
	assert(roadZero.directional == baseline.directional);
	assert(roadZero.impact == baseline.impact);
	assert(roadMaximum.directional == baseline.directional);
	assert(roadMaximum.impact == baseline.impact);

	assert(Active::mode_from_string("ROAD2_EXPERIMENTAL") == Active::Mode::Experimental);
	assert(Active::mode_from_string(RoadDetailModeUi::CanonicalValues[0]) == Active::Mode::ReferencePlus);
	assert(Active::mode_from_string(RoadDetailModeUi::CanonicalValues[1]) == Active::Mode::Experimental);
	assert(Active::mode_from_string("invalid") == Active::Mode::ReferencePlus);
	assert(Active::sanitize_development_gain(4) == 4);
	assert(Active::sanitize_development_gain(6) == 6);
	assert(Active::sanitize_development_gain(8) == 8);
	assert(Active::sanitize_development_gain(10) == 10);
	assert(Active::sanitize_development_gain(15) == 15);
	assert(Active::sanitize_development_gain(20) == 20);
	assert(Active::sanitize_development_gain(25) == 25);
	assert(Active::sanitize_development_gain(30) == 30);
	assert(Active::sanitize_development_gain(1) == 4);
	static_assert(Active::ShippingCalibrationGain == 8);
	static_assert(Active::DefaultDebugAuthorityGain == 10);
	static_assert(Active::DebugAuthorityGains == std::array{ 10, 15, 20, 25, 30 });
	for (const int gain : Active::DebugAuthorityGains)
	{
		assert(Active::sanitize_debug_authority_gain(gain) == gain);
		assert(Active::resolve_calibration_gain(true, gain) == gain);
		assert(Active::resolve_calibration_gain(false, gain) == Active::ShippingCalibrationGain);
		Active::DevelopmentGainStage selectedStage;
		assert(selectedStage.evaluate(0.001f, 1.0f, 0.1f,
			Active::resolve_calibration_gain(true, gain), true).developmentGain == gain);
	}
	assert(Active::sanitize_debug_authority_gain(12) == Active::DefaultDebugAuthorityGain);
	assert(Active::resolve_calibration_gain(true, 12) == Active::DefaultDebugAuthorityGain);
	const std::array liveCaptureAuthority{
		Active::resolve_calibration_gain(false, 10),
		Active::resolve_calibration_gain(true, 10),
		Active::resolve_calibration_gain(true, 15),
		Active::resolve_calibration_gain(true, 20),
		Active::resolve_calibration_gain(true, 25),
		Active::resolve_calibration_gain(true, 30),
		Active::resolve_calibration_gain(false, 30)
	};
	assert((liveCaptureAuthority == std::array{ 8, 10, 15, 20, 25, 30, 8 }));
	std::array<int, liveCaptureAuthority.size()> recordedSampleGains{};
	Active::DevelopmentGainStage liveCaptureStage;
	for (size_t sample = 0; sample < liveCaptureAuthority.size(); ++sample)
		recordedSampleGains[sample] = liveCaptureStage.evaluate(0.0001f, 1.0f,
			1.0f / 60.0f, liveCaptureAuthority[sample], true).developmentGain;
	assert(recordedSampleGains == liveCaptureAuthority);
	static_assert(RoadDetailModeUi::Label == "Road Detail Mode");
	static_assert(RoadDetailModeUi::Choices[0] == "Classic");
	static_assert(RoadDetailModeUi::Choices[1] == "Enhanced");
	static_assert(RoadDetailModeUi::Tooltip ==
		"Choose how road surfaces feel. Classic preserves the original feedback; "
		"Enhanced adds more detailed surface texture.");

	// Development gain is a post-Road-Detail engineering stage constrained to
	// the physically useful C1 sweep. Invalid values fall back safely to 4x.
	Active::DevelopmentGainStage fourXReplay;
	Active::DevelopmentGainStage invalidGain;
	for (size_t n = 0; n < a.size(); ++n)
	{
		const auto& four = fourXReplay.evaluate(a[n], 1.0f, 1.0f / 60.0f, 4, true);
		const auto& fallback = invalidGain.evaluate(a[n], 1.0f, 1.0f / 60.0f, 3, true);
		assert(four.finalRoad == fallback.finalRoad);
		assert(fallback.invalidGainFallback);
	}

	for (const int gain : { 4, 6, 8, 10 })
	{
		Active::DevelopmentGainStage stage;
		const auto& silence = stage.evaluate(0.0f, 1.0f, 1.0f / 60.0f, gain, false);
		assert(silence.finalRoad == 0.0f && !silence.clamped);
	}
	Active::DevelopmentGainStage fourX;
	Active::DevelopmentGainStage sixX;
	Active::DevelopmentGainStage eightX;
	Active::DevelopmentGainStage tenX;
	assert(fourX.evaluate(0.01f, 1.0f, 0.1f, 4, true).finalRoad == 0.04f);
	assert(sixX.evaluate(0.01f, 1.0f, 0.1f, 6, true).finalRoad == 0.06f);
	assert(eightX.evaluate(0.01f, 1.0f, 0.1f, 8, true).finalRoad == 0.08f);
	assert(std::abs(tenX.evaluate(0.01f, 1.0f, 0.1f, 10, true).finalRoad - 0.09f) < 1.0e-6f); // slew limited
	Active::DevelopmentGainStage pipelineObservation;
	const auto& observedPipeline = pipelineObservation.evaluate(0.02f, 1.0f, 0.1f, 30, true);
	assert(observedPipeline.developmentGain == 30);
	assert(observedPipeline.preGainRoad == 0.02f);
	assert(std::abs(observedPipeline.postGainRoad - 0.60f) < 1.0e-6f);
	assert(observedPipeline.boundedRoad == Active::RoadChannelSafetyCeiling);
	assert(observedPipeline.clamped);
	assert(observedPipeline.slewLimited);
	assert(std::abs(observedPipeline.finalRoad - Active::MaximumSlewPerSecond * 0.1f) < 1.0e-6f);
	Active::DevelopmentGainStage halfRoadDetail;
	Active::DevelopmentGainStage fullRoadDetail;
	const auto halfAmount = halfRoadDetail.evaluate(0.005f, 0.5f, 0.1f, 4, true).finalRoad;
	const auto fullAmount = fullRoadDetail.evaluate(0.01f, 1.0f, 0.1f, 4, true).finalRoad;
	assert(halfAmount * 2.0f == fullAmount); // amount changes; generator character does not
	const auto& clamped = eightX.evaluate(0.10f, 2.0f, 0.1f, 8, true);
	assert(clamped.clamped);
	assert(clamped.finalRoad <= Active::RoadChannelSafetyCeiling);
	assert(std::abs(clamped.finalRoad - 0.08f) <= Active::MaximumSlewPerSecond * 0.1f + 1.0e-6f);
	eightX.evaluate(0.10f, 2.0f, 0.1f, 8, true);
	const auto& settledClamp = eightX.evaluate(0.10f, 2.0f, 0.1f, 8, true);
	assert(settledClamp.clamped);
	assert(settledClamp.finalRoad == Active::RoadChannelSafetyCeiling);

	Active::DevelopmentGainStage highGainReplay;
	float highGainSum = 0.0f;
	float highGainPrevious = 0.0f;
	for (const float value : a)
	{
		const auto& gained = highGainReplay.evaluate(value, 1.0f, 1.0f / 60.0f, 8, true);
		assert(std::abs(gained.finalRoad) <= Active::RoadChannelSafetyCeiling);
		assert(std::abs(gained.finalRoad - highGainPrevious) <=
			Active::MaximumSlewPerSecond / 60.0f + 1.0e-6f);
		highGainSum += gained.finalRoad;
		highGainPrevious = gained.finalRoad;
	}
	assert(std::abs(highGainSum / static_cast<float>(a.size())) < 0.01f);
	return 0;
}
