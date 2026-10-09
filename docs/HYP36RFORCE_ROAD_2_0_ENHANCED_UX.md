# Road 2.0 (Enhanced) player-facing terminology

Road 2.0 remains the engineering name for the independently developed higher-detail Road presentation. In the normal Advanced Force Feedback interface, it is presented as **Enhanced** under **Road Detail Mode**.

The two player choices are:

- **Classic** — the established Reference+ Road presentation.
- **Enhanced** — Road 2.0 (Enhanced), the higher-detail Road presentation.

Road Detail Mode selects how Road is presented. The adjacent Road Detail 0–100% control continues to select how much Road is presented. Its mapping, range, Recommended marker, persistence, tooltip, and Reference+ baseline are unchanged.

The canonical `[Developer] RoadPresentation` values remain `REFERENCE_PLUS` and `ROAD2_EXPERIMENTAL`, preserving existing configurations and internal engineering lineage. The default remains `REFERENCE_PLUS`. The 1x/2x/4x/8x Development Gain remains available only in Debug → Road 2.0 Experimental and is not part of the player-facing selector.

This terminology milestone does not change either Road implementation, its generator, native authority, filtering, safety limits, output routing, or DirectInput behavior.
