# HYP36rforce Digital Dash

An experimental SimHub 9.13.2 dashboard for HYP36rforce SimHub Integration in
Multi-Input v2.0. It is not included in v1.5.0 or v1.5.1, and v2.0 has not been
publicly released.

## Install

1. Extract the downloaded development artifact once.
2. In SimHub 9.13.2, open **Dash Studio → Import dashboard** and select `HYP36rforce Digital Dash.simhubdash`. Double-click import remains supported by SimHub, but the in-app command gives clearer feedback if Windows file association is unavailable.
3. Confirm the import prompt identifies **HYP36rforce Digital Dash**, then import it into the library.
4. Select **HYP36rforce Digital Dash** as the dashboard to display.
5. In OutRun, enable **Settings → Gameplay → SimHub Telemetry**.
6. Start driving.

No manual property assignment is required. The dashboard is optional and does not affect the
underlying telemetry adapter or wheel force feedback.

Speed and Gear deliberately display `--` because SIMHUB-02B does not yet publish validated
physical speed units or gear encoding. The diagnostic strip says `Telemetry State: Unverified`
because the current contract has no receiver-confirmed validity property.

The dashboard uses only standard Windows Segoe UI and contains no proprietary fonts, logos,
vehicle artwork, or manufacturer artwork.

The title, Speed, Gear, and Steering sections are static dashboard elements and remain visible when telemetry is unavailable. Only their live values use telemetry bindings. The corrected import package contains one top-level `HYP36rforce Digital Dash` folder, matching SimHub-exported dashboard structure.

Real import and rendering in SimHub 9.13.2 still require validation. No manual-copy fallback is documented because this milestone has not independently verified an installed DashTemplates path on the target system.
