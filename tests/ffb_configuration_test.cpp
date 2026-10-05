#include "ffb_configuration.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

int main()
{
	using namespace HYP36RFFBConfiguration;

	// Fresh/missing player settings inherit the authoritative v1.5 defaults.
	State fresh;
	assert(migrate_v15(fresh));
	assert(fresh.migrationVersion == CurrentMigrationVersion);
	assert(fresh.player.enabled && fresh.player.strength == 100);
	assert(fresh.player.steeringLoad == 100 && fresh.player.roadDetail == 100 && fresh.player.impact == 100);
	assert(fresh.player.roadMode == "ROAD2_EXPERIMENTAL" && fresh.player.surface == 50 && !fresh.player.invert);
	assert(fresh.research.roadAuthority == false && fresh.research.textureCeilingOverride == false);
	assert(fresh.research.textureCeiling == 18 && fresh.research.waveform == "TRIANGLE");
	assert(fresh.research.frequency == "REFERENCE" && fresh.research.retiredBumpEnabled);
	assert(fresh.research.retiredBumpThreshold == 0.020f && fresh.research.retiredBumpStrength == 30);
	assert(fresh.research.retiredBumpDuration == 60);

	// A stable user's choices survive; a missing newer Surface value remains its default.
	State stable;
	stable.player = { false, 72, 84, 90, 66, PlayerState{}.surface,
		"REFERENCE_PLUS", true, "Active", "REFERENCE_PLUS_EXPERIMENTAL" };
	assert(migrate_v15(stable));
	assert(!stable.player.enabled && stable.player.strength == 72 && stable.player.steeringLoad == 84);
	assert(stable.player.roadDetail == 90 && stable.player.impact == 66 && stable.player.surface == 50);
	assert(stable.player.roadMode == "REFERENCE_PLUS" && stable.player.invert);

	// Pre-release research state is normalized once without touching player choices.
	State development;
	development.player = stable.player;
	development.research.roadAuthority = true;
	development.research.roadAuthorityGain = 15;
	development.research.textureCeilingOverride = true;
	development.research.textureCeiling = 25;
	development.research.waveform = "SINE";
	development.research.frequency = "HIGH";
	development.research.retiredBumpEnabled = false;
	development.research.retiredBumpThreshold = 0.08f;
	development.research.retiredBumpStrength = 8;
	development.research.retiredBumpDuration = 180;
	development.research.preferredAmplitude = 25;
	development.research.preferredWaveform = "SINE";
	assert(migrate_v15(development));
	assert(development.player.strength == stable.player.strength && development.player.invert == stable.player.invert);
	assert(!development.research.roadAuthority && development.research.roadAuthorityGain == 15);
	assert(!development.research.textureCeilingOverride && development.research.textureCeiling == 18);
	assert(development.research.waveform == "TRIANGLE" && development.research.frequency == "REFERENCE");
	assert(development.research.retiredBumpEnabled && development.research.retiredBumpStrength == 30);
	assert(development.research.preferredAmplitude == 25 && development.research.preferredWaveform == "SINE");

	// Deliberate post-migration Debug choices persist because migration cannot run twice.
	development.research.roadAuthority = true;
	development.research.roadAuthorityGain = 25;
	development.research.waveform = "SQUARE";
	development.research.frequency = "MEDIUM";
	assert(!migrate_v15(development));
	assert(development.research.roadAuthority && development.research.roadAuthorityGain == 25);
	assert(development.research.waveform == "SQUARE" && development.research.frequency == "MEDIUM");

	// Player reset restores the complete FFB baseline and clears hidden research overrides.
	State broken = development;
	broken.player = { false, 3, 4, 5, 6, 7, "REFERENCE_PLUS", true, "Legacy", "REFERENCE" };
	reset_player(broken);
	assert(broken.player.enabled && broken.player.strength == 100);
	assert(broken.player.steeringLoad == 100 && broken.player.roadDetail == 100 && broken.player.impact == 100);
	assert(broken.player.surface == 50 && broken.player.roadMode == "ROAD2_EXPERIMENTAL" && !broken.player.invert);
	assert(broken.player.forceMode == "Active" && broken.player.presentationMode == "REFERENCE_PLUS_EXPERIMENTAL");
	assert(!broken.research.roadAuthority && !broken.research.textureCeilingOverride);
	assert(broken.research.waveform == "TRIANGLE" && broken.research.frequency == "REFERENCE");

	// Debug reset is isolated from every player-facing preference.
	State debugOnly = development;
	const PlayerState playerBefore = debugOnly.player;
	reset_debug(debugOnly);
	assert(debugOnly.player.enabled == playerBefore.enabled && debugOnly.player.strength == playerBefore.strength);
	assert(debugOnly.player.steeringLoad == playerBefore.steeringLoad && debugOnly.player.roadDetail == playerBefore.roadDetail);
	assert(debugOnly.player.impact == playerBefore.impact && debugOnly.player.surface == playerBefore.surface);
	assert(debugOnly.player.roadMode == playerBefore.roadMode && debugOnly.player.invert == playerBefore.invert);
	assert(!debugOnly.research.roadAuthority && !debugOnly.research.textureCeilingOverride);
	assert(debugOnly.research.waveform == "TRIANGLE" && debugOnly.research.frequency == "REFERENCE");
}
