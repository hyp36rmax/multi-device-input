#include "force_character_presentation.hpp"
#include "road2_active.hpp"

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
		value.spatial.differingFromReference = 4;
		value.spatial.coverage = Policy::Coverage::Broad;
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
	assert(Active::mode_from_string("invalid") == Active::Mode::ReferencePlus);
	return 0;
}
