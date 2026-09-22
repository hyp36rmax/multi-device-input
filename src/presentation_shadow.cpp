#include "presentation_shadow.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>

#include <spdlog/spdlog.h>

#include "settings.hpp"

namespace Settings
{
	Setting<std::string> PresentationMode{ "Developer", "PresentationMode", "REFERENCE_PLUS_EXPERIMENTAL",
		"HYP36R presentation mode: REFERENCE or REFERENCE_PLUS_EXPERIMENTAL." };

	namespace
	{
		struct HidePresentationSettings
		{
			HidePresentationSettings()
			{
				PresentationMode.hidden(true);
			}
		} hidePresentationSettings;
	}
}

namespace HYP36RPresentation
{
	namespace
	{
		struct Configuration
		{
			Profile profile = Profile::Reference;
			Mode mode = Mode::Reference;
			float presence = 1.0f;
			float contrast = 0.0f;
			float budgetFraction = 0.05f;
			bool fallback = false;
		};

		Configuration configuration{};
		Frame current{};

		std::string_view trim_ascii_whitespace(std::string_view value)
		{
			while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())))
				value.remove_prefix(1);
			while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())))
				value.remove_suffix(1);
			return value;
		}

		bool equals_ascii_case_insensitive(std::string_view left, std::string_view right)
		{
			if (left.size() != right.size())
				return false;
			for (size_t index = 0; index < left.size(); ++index)
			{
				const auto a = static_cast<unsigned char>(left[index]);
				const auto b = static_cast<unsigned char>(right[index]);
				if (std::tolower(a) != std::tolower(b))
					return false;
			}
			return true;
		}

		SoftwareRegion classify_region(float presence)
		{
			if (presence <= 1.3f)
				return SoftwareRegion::Comfort;
			if (presence <= 1.6f)
				return SoftwareRegion::HeadroomAware;
			if (presence < 2.0f)
				return SoftwareRegion::CompressionRisk;
			return SoftwareRegion::SaturationRisk;
		}

		float finite_or_zero(float value)
		{
			return std::isfinite(value) ? value : 0.0f;
		}
	}

	void initialize()
	{
		Configuration next{};
		const auto mode = trim_ascii_whitespace(Settings::PresentationMode.get());
		if (equals_ascii_case_insensitive(mode, "REFERENCE_PLUS_EXPERIMENTAL"))
		{
			next.mode = Mode::ReferencePlusExperimental;
			// Fifteen percent more Reference+ directional presentation than S9.
			// Reference and all upstream M4/M5 policy remain unchanged.
			next.presence = 1.38f;
			next.contrast = 4.0f;
		}
		else if (!equals_ascii_case_insensitive(mode, "REFERENCE"))
		{
			next.fallback = true;
		}
		configuration = next;
		reset();
		spdlog::info("HYP36R presentation: mode={} profile={} presence={:.2f} contrast={:g} budget={:.3f}{}",
			mode_name(configuration.mode), profile_name(configuration.profile), configuration.presence,
			configuration.contrast, configuration.budgetFraction,
			configuration.fallback ? " fallback=Reference" : "");
	}

	const Frame& evaluate(const Inputs& rawInputs)
	{
		Frame next{};
		next.profile = configuration.profile;
		next.mode = configuration.mode;
		next.presence = configuration.presence;
		next.contrast = configuration.contrast;
		next.secondaryBudgetFraction = configuration.budgetFraction;
		next.fallbackActive = configuration.fallback;
		next.softwareRegion = classify_region(configuration.presence);
		const bool finiteInputs = std::isfinite(rawInputs.directionalPrimary) &&
			std::isfinite(rawInputs.directionalSecondaryRaw) &&
			std::isfinite(rawInputs.legacyDirectional) &&
			std::isfinite(rawInputs.road) && std::isfinite(rawInputs.impact) &&
			std::isfinite(rawInputs.vibration);
		if (!finiteInputs ||
			(next.mode == Mode::ReferencePlusExperimental && !rawInputs.m4Active))
		{
			next.mode = Mode::Reference;
			next.presence = 1.0f;
			next.contrast = 0.0f;
			next.fallbackActive = true;
		}
		next.directionalPrimary = finite_or_zero(rawInputs.directionalPrimary);
		next.roadRequest = finite_or_zero(rawInputs.road);
		next.impactRequest = finite_or_zero(rawInputs.impact);
		next.vibrationRequest = finite_or_zero(rawInputs.vibration);

		const float legacyLimit = std::abs(finite_or_zero(rawInputs.legacyDirectional));
		if (rawInputs.secondaryEligible && !next.fallbackActive &&
			next.mode == Mode::ReferencePlusExperimental)
			next.secondaryRaw = finite_or_zero(rawInputs.directionalSecondaryRaw);
		next.secondaryRequested = next.secondaryRaw * next.contrast;

		const float budget = configuration.budgetFraction * std::abs(next.directionalPrimary);
		const float budgeted = (std::clamp)(next.secondaryRequested, -budget, budget);
		next.secondaryBudgetActive = budgeted != next.secondaryRequested;

		float semanticDirectional = next.directionalPrimary + budgeted;
		if (next.directionalPrimary == 0.0f ||
			semanticDirectional * next.directionalPrimary < 0.0f)
		{
			semanticDirectional = next.directionalPrimary;
		}

		// The Legacy boundary constrains only the added secondary information. It
		// must never reduce an already-authoritative M4 primary at Contrast zero.
		const float legacyBounded = std::abs(next.directionalPrimary) > legacyLimit
			? next.directionalPrimary
			: (std::clamp)(semanticDirectional, -legacyLimit, legacyLimit);
		next.legacyBoundaryActive = legacyBounded != semanticDirectional;
		next.secondaryPermitted = legacyBounded - next.directionalPrimary;
		next.directionalPrePresence = legacyBounded;
		next.directionalRequest = finite_or_zero(legacyBounded * next.presence);
		next.hardwareDirectionalSelected = next.mode == Mode::ReferencePlusExperimental
			? next.directionalRequest : next.directionalPrimary;
		if (!std::isfinite(next.hardwareDirectionalSelected))
		{
			next.mode = Mode::Reference;
			next.presence = 1.0f;
			next.contrast = 0.0f;
			next.secondaryRaw = 0.0f;
			next.secondaryRequested = 0.0f;
			next.secondaryPermitted = 0.0f;
			next.directionalPrePresence = next.directionalPrimary;
			next.directionalRequest = next.directionalPrimary;
			next.hardwareDirectionalSelected = next.directionalPrimary;
			next.secondaryBudgetActive = false;
			next.legacyBoundaryActive = false;
			next.fallbackActive = true;
			next.softwareRegion = SoftwareRegion::Comfort;
		}

		current = next;
		return current;
	}

	void reset()
	{
		current = {};
		current.profile = configuration.profile;
		current.mode = configuration.mode;
		current.presence = configuration.presence;
		current.contrast = configuration.contrast;
		current.secondaryBudgetFraction = configuration.budgetFraction;
		current.fallbackActive = configuration.fallback;
		current.softwareRegion = classify_region(configuration.presence);
	}

	const Frame& frame() { return current; }
	const char* profile_name(Profile) { return "Reference"; }
	const char* mode_name(Mode mode)
	{
		return mode == Mode::ReferencePlusExperimental
			? "REFERENCE_PLUS_EXPERIMENTAL" : "REFERENCE";
	}

	const char* software_region_name(SoftwareRegion region)
	{
		switch (region) {
		case SoftwareRegion::HeadroomAware: return "headroom-aware";
		case SoftwareRegion::CompressionRisk: return "compression-risk";
		case SoftwareRegion::SaturationRisk: return "saturation-risk";
		default: return "comfort";
		}
	}
}
