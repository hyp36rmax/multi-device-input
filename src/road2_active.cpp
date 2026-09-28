#include "road2_active.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>

namespace HYP36RRoad2Active
{
	namespace
	{
		constexpr double FixedStepSeconds = 1.0 / 120.0;
		constexpr float AttackPerSecond = 7.5f;
		constexpr float ReleasePerSecond = 11.0f;
		constexpr float DcPole = 0.992f;
		Generator RuntimeGenerator{};

		float move_towards(float current, float target, float maximumDelta)
		{
			if (current < target)
				return (std::min)(target, current + maximumDelta);
			return (std::max)(target, current - maximumDelta);
		}

		bool finite_frame(const HYP36RRoad2::Frame& policy,
			const HYP36RRoad2Presentation::Frame& presentation, float delta)
		{
			return std::isfinite(delta) && delta > 0.0f && delta <= 0.1f &&
				std::isfinite(policy.activity.leftEvidence) &&
				std::isfinite(policy.activity.rightEvidence) &&
				std::isfinite(presentation.continuous.normalized);
		}
	}

	uint32_t Generator::next_random() noexcept
	{
		uint32_t value = randomState_;
		value ^= value << 13;
		value ^= value >> 17;
		value ^= value << 5;
		randomState_ = value ? value : DeterministicSeed;
		return randomState_;
	}

	void Generator::advance_generator() noexcept
	{
		const float white = (static_cast<float>(next_random() & 0x00ffffffu) /
			static_cast<float>(0x007fffffu)) - 1.0f;
		// Two one-pole filters form a conservative band-pass texture. A final DC
		// blocker removes long-term steering bias without assigning a surface Hz.
		fastLowPass_ += 0.34f * (white - fastLowPass_);
		slowLowPass_ += 0.055f * (white - slowLowPass_);
		const float bandPass = fastLowPass_ - slowLowPass_;
		const float dcBlocked = bandPass - dcPreviousInput_ + DcPole * dcPreviousOutput_;
		dcPreviousInput_ = bandPass;
		dcPreviousOutput_ = (std::clamp)(dcBlocked, -1.0f, 1.0f);
	}

	Frame Generator::fail_safe(Mode mode, uint64_t frameId) noexcept
	{
		reset();
		current_.mode = mode;
		current_.frameId = frameId;
		current_.phase = Phase::FailSafe;
		current_.safety = SafetyState::InvalidInput;
		current_.reset = true;
		return current_;
	}

	const Frame& Generator::evaluate(const HYP36RRoad2::Frame& policy,
		const HYP36RRoad2Presentation::Frame& presentation,
		float deltaTimeSeconds, Mode mode) noexcept
	{
		if (!finite_frame(policy, presentation, deltaTimeSeconds))
		{
			fail_safe(mode, policy.frameId);
			return current_;
		}

		Frame next{};
		next.mode = mode;
		next.frameId = policy.frameId;
		const bool authorized = mode == Mode::Experimental &&
			policy.meta.validity == HYP36RSignalState::Validity::Valid &&
			policy.activity.state == HYP36RRoad2::ActivityState::ContinuousCandidate &&
			presentation.continuous.active;
		next.nativeAuthority = authorized
			? (std::clamp)(presentation.continuous.normalized, 0.0f, 1.0f) : 0.0f;
		const float coverage = authorized
			? (std::clamp)(static_cast<float>(policy.spatial.differingFromReference) * 0.25f,
				0.0f, 1.0f) : 0.0f;
		next.occupancyTarget = coverage;
		const float previousEnvelope = occupancyEnvelope_;
		fixedAccumulator_ += static_cast<double>(deltaTimeSeconds);
		while (fixedAccumulator_ + 1.0e-10 >= FixedStepSeconds)
		{
			const float envelopeRate = coverage > occupancyEnvelope_
				? AttackPerSecond : ReleasePerSecond;
			occupancyEnvelope_ = move_towards(occupancyEnvelope_, coverage,
				envelopeRate * static_cast<float>(FixedStepSeconds));
			advance_generator();
			if (next.nativeAuthority > 0.0f)
			{
				const float target = (std::clamp)(next.nativeAuthority * occupancyEnvelope_ *
					dcPreviousOutput_ * InternalCeiling, -InternalCeiling, InternalCeiling);
				previousContribution_ = move_towards(previousContribution_, target,
					MaximumSlewPerSecond * static_cast<float>(FixedStepSeconds));
			}
			fixedAccumulator_ -= FixedStepSeconds;
		}
		next.occupancyEnvelope = occupancyEnvelope_;
		next.phase = occupancyEnvelope_ > previousEnvelope + 1.0e-6f ? Phase::Attack
			: occupancyEnvelope_ + 1.0e-6f < previousEnvelope ? Phase::Release
			: coverage > 0.0f ? Phase::Sustain : Phase::Quiet;
		next.rawGenerator = fastLowPass_ - slowLowPass_;
		next.conditionedTexture = dcPreviousOutput_;

		if (next.nativeAuthority <= 0.0f)
		{
			previousContribution_ = 0.0f;
			next.safety = SafetyState::AuthorityZero;
			current_ = next;
			return current_;
		}

		next.unclampedContribution = next.nativeAuthority * next.occupancyEnvelope *
			next.conditionedTexture * InternalCeiling;
		float bounded = (std::clamp)(next.unclampedContribution, -InternalCeiling, InternalCeiling);
		if (bounded != next.unclampedContribution)
		{
			next.clamped = true;
			next.safety = SafetyState::OutputClamped;
		}
		if (std::abs(previousContribution_ - bounded) > 1.0e-7f)
		{
			next.slewLimited = true;
			next.safety = SafetyState::SlewLimited;
		}
		next.contribution = (std::clamp)(previousContribution_, -InternalCeiling, InternalCeiling);
		if (!std::isfinite(next.contribution))
		{
			fail_safe(mode, policy.frameId);
			return current_;
		}
		next.active = true;
		current_ = next;
		return current_;
	}

	void Generator::reset() noexcept
	{
		current_ = {};
		randomState_ = DeterministicSeed;
		fixedAccumulator_ = 0.0;
		fastLowPass_ = 0.0f;
		slowLowPass_ = 0.0f;
		dcPreviousInput_ = 0.0f;
		dcPreviousOutput_ = 0.0f;
		occupancyEnvelope_ = 0.0f;
		previousContribution_ = 0.0f;
	}

	Mode mode_from_string(std::string_view value) noexcept
	{
		std::string normalized(value);
		for (char& character : normalized)
			character = static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
		return normalized == "ROAD2_EXPERIMENTAL" || normalized == "ROAD 2.0 EXPERIMENTAL"
			? Mode::Experimental : Mode::ReferencePlus;
	}

	const char* mode_name(Mode mode) noexcept
	{
		return mode == Mode::Experimental ? "Road 2.0 Experimental" : "Reference+";
	}

	const char* phase_name(Phase phase) noexcept
	{
		switch (phase)
		{
		case Phase::Attack: return "Attack";
		case Phase::Sustain: return "Sustain";
		case Phase::Release: return "Release";
		case Phase::FailSafe: return "Fail-safe";
		default: return "Quiet";
		}
	}

	const char* safety_name(SafetyState safety) noexcept
	{
		switch (safety)
		{
		case SafetyState::AuthorityZero: return "Authority zero";
		case SafetyState::InvalidInput: return "Invalid input";
		case SafetyState::OutputClamped: return "Output clamped";
		case SafetyState::SlewLimited: return "Slew limited";
		default: return "Clear";
		}
	}

	float select_road(Mode mode, float referenceRoad, float experimentalRoad) noexcept
	{
		if (mode == Mode::ReferencePlus)
			return referenceRoad;
		return std::isfinite(experimentalRoad) ? experimentalRoad : 0.0f;
	}

	const Frame& evaluate(const HYP36RRoad2::Frame& policy,
		const HYP36RRoad2Presentation::Frame& presentation,
		float deltaTimeSeconds, Mode mode) noexcept
	{
		return RuntimeGenerator.evaluate(policy, presentation, deltaTimeSeconds, mode);
	}
	void reset() noexcept { RuntimeGenerator.reset(); }
	const Frame& frame() noexcept { return RuntimeGenerator.frame(); }
}
