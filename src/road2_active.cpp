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
		constexpr float TwoPi = 6.28318530717958647692f;
		Generator RuntimeGenerator{};
		DevelopmentGainStage RuntimeGainStage{};

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
		whiteNoise_ = white;
		// Two one-pole filters form a conservative band-pass texture. A final DC
		// blocker removes long-term steering bias without assigning a surface Hz.
		fastLowPass_ += 0.34f * (white - fastLowPass_);
		slowLowPass_ += 0.055f * (white - slowLowPass_);
		const float bandPass = fastLowPass_ - slowLowPass_;
		const float dcBlocked = bandPass - dcPreviousInput_ + DcPole * dcPreviousOutput_;
		dcPreviousInput_ = bandPass;
		dcPreviousOutput_ = (std::clamp)(dcBlocked, -1.0f, 1.0f);
		roughLowPass_ += 0.08f * ((fastLowPass_ - slowLowPass_) - roughLowPass_);
		// Presentation cadence only: this is not a recovered native material
		// frequency. Deterministic noise keeps the tight component from becoming
		// a perfectly repeating controller-style buzz.
		characterPhase_ += 0.82f + 0.08f * white;
		if (characterPhase_ >= TwoPi)
			characterPhase_ -= TwoPi;
	}

	SurfaceArchetype classify_surface(const HYP36RRoad2::Frame& policy) noexcept
	{
		if (!policy.spatial.referenceEstablished || policy.spatial.differingFromReference == 0)
			return SurfaceArchetype::None;
		const std::array<uint32_t, 4> surfaces{
			policy.spatial.group02.raw[0], policy.spatial.group13.raw[0],
			policy.spatial.group02.raw[1], policy.spatial.group13.raw[1]
		};
		std::array<int, 3> counts{};
		int unknown = 0;
		for (const uint32_t surface : surfaces)
		{
			if (surface == policy.spatial.referenceRaw)
				continue;
			if (surface == 0x100000u)
				++counts[0];
			else if (surface == 4u || surface == 8u || surface == 0x2000u)
				++counts[1];
			else if (surface == 0x400u || surface == 0x800u)
				++counts[2];
			else
				++unknown;
		}
		const int best = (std::max)({ counts[0], counts[1], counts[2] });
		if (best == 0 || unknown >= best)
			return SurfaceArchetype::GenericEnhanced;
		if (std::count(counts.begin(), counts.end(), best) != 1)
			return SurfaceArchetype::GenericEnhanced;
		return counts[0] == best ? SurfaceArchetype::HardUneven
			: counts[1] == best ? SurfaceArchetype::SoftRough
			: SurfaceArchetype::StripedRunoff;
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
		next.archetype = classify_surface(policy);
		next.frameId = policy.frameId;
		const bool authorized = mode == Mode::Experimental &&
			policy.meta.validity == HYP36RSignalState::Validity::Valid &&
			policy.activity.state == HYP36RRoad2::ActivityState::ContinuousCandidate &&
			presentation.continuous.active;
		next.nativeAuthority = authorized
			? (std::clamp)(presentation.continuous.normalized, 0.0f, 1.0f) : 0.0f;
		// Retain native amplitude ownership while lifting weak-but-authorized
		// surfaces conservatively toward audibility. This does not flatten surfaces.
		next.presentationAuthority = next.nativeAuthority > 0.0f
			? 0.70f * next.nativeAuthority + 0.30f * std::sqrt(next.nativeAuthority) : 0.0f;
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
				float character = 0.0f;
				switch (next.archetype)
				{
				case SurfaceArchetype::HardUneven:
					character = std::sin(characterPhase_) *
						(0.75f + 0.25f * std::abs(dcPreviousOutput_)) * 0.22f;
					break;
				case SurfaceArchetype::SoftRough:
					character = roughLowPass_ * 0.18f;
					break;
				case SurfaceArchetype::StripedRunoff:
					character = (whiteNoise_ - fastLowPass_) * 0.14f;
					break;
				default:
					break;
				}
				const float target = (std::clamp)(next.presentationAuthority * occupancyEnvelope_ *
					(dcPreviousOutput_ + character) * InternalCeiling,
					-InternalCeiling, InternalCeiling);
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
		next.aperiodicBase = next.presentationAuthority * next.occupancyEnvelope *
			next.conditionedTexture * InternalCeiling;
		switch (next.archetype)
		{
		case SurfaceArchetype::HardUneven:
			next.characterComponent = next.presentationAuthority * next.occupancyEnvelope *
				std::sin(characterPhase_) * (0.75f + 0.25f * std::abs(dcPreviousOutput_)) *
				0.22f * InternalCeiling;
			break;
		case SurfaceArchetype::SoftRough:
			next.characterComponent = next.presentationAuthority * next.occupancyEnvelope *
				roughLowPass_ * 0.18f * InternalCeiling;
			break;
		case SurfaceArchetype::StripedRunoff:
			next.characterComponent = next.presentationAuthority * next.occupancyEnvelope *
				(whiteNoise_ - fastLowPass_) * 0.14f * InternalCeiling;
			break;
		default:
			break;
		}
		next.preSafetyContribution = next.aperiodicBase + next.characterComponent;

		if (next.nativeAuthority <= 0.0f)
		{
			previousContribution_ = 0.0f;
			next.safety = SafetyState::AuthorityZero;
			current_ = next;
			return current_;
		}

		next.unclampedContribution = next.preSafetyContribution;
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
		whiteNoise_ = 0.0f;
		roughLowPass_ = 0.0f;
		characterPhase_ = 0.0f;
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

	const char* archetype_name(SurfaceArchetype archetype) noexcept
	{
		switch (archetype)
		{
		case SurfaceArchetype::HardUneven: return "Hard / uneven";
		case SurfaceArchetype::SoftRough: return "Soft / rough";
		case SurfaceArchetype::StripedRunoff: return "Striped / runoff";
		case SurfaceArchetype::GenericEnhanced: return "Generic Enhanced";
		default: return "None";
		}
	}

	float select_road(Mode mode, float referenceRoad, float experimentalRoad) noexcept
	{
		if (mode == Mode::ReferencePlus)
			return referenceRoad;
		return std::isfinite(experimentalRoad) ? experimentalRoad : 0.0f;
	}

	int sanitize_development_gain(int requestedGain) noexcept
	{
		switch (requestedGain)
		{
		case 4: case 6: case 8: case 10: return requestedGain;
		default: return 4;
		}
	}

	const GainFrame& DevelopmentGainStage::evaluate(float roadAfterDetail,
		float roadDetailScale, float deltaTimeSeconds, int requestedGain,
		bool nativeAuthorized) noexcept
	{
		GainFrame next{};
		next.developmentGain = sanitize_development_gain(requestedGain);
		next.invalidGainFallback = next.developmentGain != requestedGain;
		next.roadDetailScale = std::isfinite(roadDetailScale)
			? (std::max)(0.0f, roadDetailScale) : 0.0f;
		next.preGainRoad = std::isfinite(roadAfterDetail) ? roadAfterDetail : 0.0f;
		if (!nativeAuthorized)
		{
			previousRoad_ = 0.0f;
			current_ = next;
			return current_;
		}
		if (!std::isfinite(roadAfterDetail) || !std::isfinite(roadDetailScale) ||
			!std::isfinite(deltaTimeSeconds) || deltaTimeSeconds <= 0.0f ||
			deltaTimeSeconds > 0.1f)
		{
			previousRoad_ = 0.0f;
			current_ = next;
			return current_;
		}

		next.postGainRoad = next.preGainRoad * static_cast<float>(next.developmentGain);
		const float bounded = (std::clamp)(next.postGainRoad,
			-RoadChannelSafetyCeiling, RoadChannelSafetyCeiling);
		next.clamped = bounded != next.postGainRoad;
		const float maximumDelta = MaximumSlewPerSecond * deltaTimeSeconds;
		next.finalRoad = move_towards(previousRoad_, bounded, maximumDelta);
		next.slewLimited = std::abs(next.finalRoad - bounded) > 1.0e-7f;
		if (!std::isfinite(next.finalRoad))
			next.finalRoad = 0.0f;
		previousRoad_ = next.finalRoad;
		current_ = next;
		return current_;
	}

	void DevelopmentGainStage::reset() noexcept
	{
		current_ = {};
		previousRoad_ = 0.0f;
	}

	const Frame& evaluate(const HYP36RRoad2::Frame& policy,
		const HYP36RRoad2Presentation::Frame& presentation,
		float deltaTimeSeconds, Mode mode) noexcept
	{
		return RuntimeGenerator.evaluate(policy, presentation, deltaTimeSeconds, mode);
	}
	void reset() noexcept { RuntimeGenerator.reset(); }
	const Frame& frame() noexcept { return RuntimeGenerator.frame(); }
	const GainFrame& evaluate_gain(float roadAfterDetail, float roadDetailScale,
		float deltaTimeSeconds, int requestedGain, bool nativeAuthorized) noexcept
	{
		return RuntimeGainStage.evaluate(roadAfterDetail, roadDetailScale,
			deltaTimeSeconds, requestedGain, nativeAuthorized);
	}
	void reset_gain() noexcept { RuntimeGainStage.reset(); }
	const GainFrame& gain_frame() noexcept { return RuntimeGainStage.frame(); }
}
