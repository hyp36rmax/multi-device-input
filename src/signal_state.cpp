#include "signal_state.hpp"

#include <algorithm>
#include <cmath>

namespace HYP36RSignalState
{
	namespace
	{
		PassiveObserver RuntimeObserver{};

		SignalMeta valid_meta(Source source, Confidence confidence, uint64_t frame)
		{
			return { Validity::Valid, source, confidence, FallbackReason::None, frame, 0 };
		}

		SignalMeta invalid_meta(Validity validity, FallbackReason fallback, uint64_t frame)
		{
			return { validity, Source::None, Confidence::Unknown, fallback, frame, 0 };
		}

		bool finite_vehicle(const Inputs& i)
		{
			return std::isfinite(i.steering) && std::isfinite(i.speed) &&
				std::isfinite(i.normalizedSpeed) && std::isfinite(i.responseAngle) &&
				std::isfinite(i.responseRate) && std::isfinite(i.referenceResponseError) &&
				std::isfinite(i.responseAuthority);
		}

		bool finite_effect(const Inputs& i)
		{
			return std::isfinite(i.effectLeft) && std::isfinite(i.effectRight) &&
				std::isfinite(i.effectCombined) && std::isfinite(i.effectRise) &&
				std::isfinite(i.existingImpact);
		}

		SpatialOccupancy occupancy_of(const std::array<uint32_t, CornerCount>& s)
		{
			SpatialOccupancy result{};
			std::array<uint32_t, CornerCount> unique{};
			for (const auto value : s)
			{
				bool found = false;
				for (uint8_t n = 0; n < result.distinctCount; ++n)
					found = found || unique[n] == value;
				if (!found)
					unique[result.distinctCount++] = value;
			}
			result.allSame = result.distinctCount <= 1;
			result.mixed = !result.allSame;
			result.pair02Same = s[0] == s[2];
			result.pair13Same = s[1] == s[3];
			return result;
		}

		void make_stale(SignalMeta& meta, uint32_t age)
		{
			if (meta.validity == Validity::Valid)
			{
				meta.validity = Validity::Stale;
				meta.fallback = FallbackReason::Stale;
				meta.confidence = Confidence::Unknown;
				meta.ageFrames = age;
			}
		}

		void set_age(SignalMeta& meta, uint32_t age)
		{
			meta.ageFrames = age;
		}
	}

	const Frame& PassiveObserver::observe(const Inputs& i) noexcept
	{
		Frame next{};
		next.frameId = i.frameId;
		if (!i.sourceSupported)
		{
			const auto meta = invalid_meta(Validity::UnsupportedSource,
				FallbackReason::UnsupportedSource, i.frameId);
			next.vehicle.meta = next.surfaces.meta = next.nativeEffect.meta = meta;
			next.gear.meta = next.grip.meta = next.dynamics.meta = meta;
			next.roadIntent.meta = next.eventIntent.meta = meta;
			current_ = next;
			return current_;
		}

		if (finite_vehicle(i))
		{
			next.vehicle.meta = valid_meta(i.nativeVehicleValid ? Source::NativeVehicleState :
				Source::CurrentRuntime, i.nativeVehicleValid ? Confidence::High : Confidence::Moderate,
				i.frameId);
			next.vehicle.steering = i.steering;
			next.vehicle.speed = i.speed;
			next.vehicle.normalizedSpeed = i.normalizedSpeed;
			if (i.nativeVehicleValid)
			{
				next.vehicle.responseAngle = i.responseAngle;
				next.vehicle.responseRate = i.responseRate;
				next.vehicle.referenceResponseError = i.referenceResponseError;
				next.vehicle.responseAuthority = i.responseAuthority;
			}
		}
		else
			next.vehicle.meta = invalid_meta(Validity::Nonfinite, FallbackReason::Nonfinite, i.frameId);

		next.surfaces.meta = valid_meta(Source::NativeSurfaceState, Confidence::High, i.frameId);
		next.surfaces.current = i.surfaces;
		next.surfaces.previous = havePreviousSurfaces_ ? previousSurfaces_ : i.surfaces;
		for (size_t n = 0; n < CornerCount; ++n)
			next.surfaces.changed[n] = havePreviousSurfaces_ && previousSurfaces_[n] != i.surfaces[n];
		if (i.field14Available)
		{
			next.surfaces.field14ValidationAvailable = true;
			for (size_t n = 0; n < CornerCount; ++n)
				if (i.field14[n] != i.surfaces[n])
					next.surfaces.field14MismatchMask |= static_cast<uint8_t>(1u << n);
			next.surfaces.field14MatchesCanonical = next.surfaces.field14MismatchMask == 0;
		}
		previousSurfaces_ = i.surfaces;
		havePreviousSurfaces_ = true;
		next.occupancy = occupancy_of(i.surfaces);

		if (finite_effect(i))
		{
			next.nativeEffect.meta = valid_meta(Source::RestoredNativeEffect, Confidence::High, i.frameId);
			next.nativeEffect.left = i.effectLeft;
			next.nativeEffect.right = i.effectRight;
			next.nativeEffect.combined = i.effectCombined;
			next.nativeEffect.rise = i.effectRise;
		}
		else
			next.nativeEffect.meta = invalid_meta(Validity::Nonfinite, FallbackReason::Nonfinite, i.frameId);

		next.gear.meta = valid_meta(Source::CurrentRuntime, Confidence::High, i.frameId);
		next.gear.current = i.currentGear;
		next.gear.previous = i.previousGear;
		next.gear.transition = i.gearTransition;
		next.grip.meta = i.gripState == GripState::Unavailable
			? invalid_meta(Validity::Unavailable, FallbackReason::Missing, i.frameId)
			: valid_meta(Source::DerivedEvidenceSupported, Confidence::Moderate, i.frameId);
		next.grip.state = i.gripState;

		bool finiteDynamics = true;
		for (const auto value : i.fieldE8)
			finiteDynamics = finiteDynamics && std::isfinite(value);
		if (i.nativeDynamicsAvailable && finiteDynamics)
		{
			next.dynamics.meta = valid_meta(Source::NativeFourCorner, Confidence::Possible, i.frameId);
			next.dynamics.fieldE8 = i.fieldE8;
			next.dynamics.fieldEC = i.fieldEC;
			next.dynamics.fieldEE = i.fieldEE;
		}
		else if (i.nativeDynamicsAvailable)
			next.dynamics.meta = invalid_meta(Validity::Nonfinite, FallbackReason::Nonfinite, i.frameId);

		if (next.nativeEffect.meta.validity == Validity::Valid)
		{
			next.roadIntent.meta = valid_meta(Source::DerivedEvidenceSupported, Confidence::Possible, i.frameId);
			next.roadIntent.occupancy = next.occupancy;
			next.roadIntent.continuousActivityEvidence = next.nativeEffect.right;
			for (const auto changed : next.surfaces.changed)
				next.roadIntent.surfaceTransitionObserved = next.roadIntent.surfaceTransitionObserved || changed;

			next.eventIntent.meta = valid_meta(Source::DerivedEvidenceSupported, Confidence::Possible, i.frameId);
			next.eventIntent.leftEvidence = i.effectLeft;
			next.eventIntent.rightEvidence = i.effectRight;
			next.eventIntent.riseEvidence = i.effectRise;
			next.eventIntent.existingImpactEvidence = i.existingImpact;
			next.eventIntent.gearTransition = i.gearTransition;
			if (i.gearTransition)
			{
				next.eventIntent.classification = EventClass::GearShift;
				next.eventIntent.meta.confidence = Confidence::High;
			}
			else if (i.effectRise > 0.12f && i.effectLeft > 0.0f && i.effectRight > 0.0f &&
				std::abs(i.existingImpact) > 0.0f)
			{
				next.eventIntent.classification = EventClass::CollisionCandidate;
				next.eventIntent.meta.confidence = Confidence::Moderate;
			}
			else if (i.effectRise > 0.12f)
				next.eventIntent.classification = EventClass::UnknownNativeEvent;
		}
		else
		{
			next.roadIntent.meta = next.nativeEffect.meta;
			next.eventIntent.meta = next.nativeEffect.meta;
		}

		current_ = next;
		return current_;
	}

	Frame PassiveObserver::snapshot(uint64_t currentFrame, uint32_t maxAgeFrames) const noexcept
	{
		Frame copy = current_;
		const auto age64 = currentFrame >= copy.frameId ? currentFrame - copy.frameId : 0;
		const auto age = static_cast<uint32_t>((std::min)(age64, uint64_t{ UINT32_MAX }));
		set_age(copy.vehicle.meta, age);
		set_age(copy.surfaces.meta, age);
		set_age(copy.nativeEffect.meta, age);
		set_age(copy.gear.meta, age);
		set_age(copy.grip.meta, age);
		set_age(copy.dynamics.meta, age);
		set_age(copy.roadIntent.meta, age);
		set_age(copy.eventIntent.meta, age);
		if (age > maxAgeFrames)
		{
			make_stale(copy.vehicle.meta, age);
			make_stale(copy.surfaces.meta, age);
			make_stale(copy.nativeEffect.meta, age);
			make_stale(copy.gear.meta, age);
			make_stale(copy.grip.meta, age);
			make_stale(copy.dynamics.meta, age);
			make_stale(copy.roadIntent.meta, age);
			make_stale(copy.eventIntent.meta, age);
			copy.roadIntent.continuousActivityEvidence = 0.0f;
			copy.roadIntent.surfaceTransitionObserved = false;
			copy.eventIntent = {};
			copy.eventIntent.meta = invalid_meta(Validity::Stale, FallbackReason::Stale,
				copy.frameId);
			copy.eventIntent.meta.ageFrames = age;
		}
		return copy;
	}

	void PassiveObserver::reset() noexcept
	{
		current_ = {};
		previousSurfaces_ = {};
		havePreviousSurfaces_ = false;
	}

	const Frame& update(const Inputs& inputs) noexcept { return RuntimeObserver.observe(inputs); }
	Frame snapshot(uint64_t currentFrame, uint32_t maxAgeFrames) noexcept
	{
		return RuntimeObserver.snapshot(currentFrame, maxAgeFrames);
	}
	void reset() noexcept { RuntimeObserver.reset(); }
	const Frame& frame() noexcept { return RuntimeObserver.frame(); }
}
