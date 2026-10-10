# HYP36rforce Digital Dash

A standalone SimHub 9.13.2 dashboard for the live OutRun 2006 external-simulation adapter.

## Install

1. Import `HYP36rforce Digital Dash.simhubdash` in SimHub Dash Studio.
2. Select **HYP36rforce Digital Dash** as the dashboard to display.
3. In OutRun, enable **Settings → Gameplay → SimHub Telemetry**.
4. Start driving.

No manual property assignment is required. The dashboard is optional and does not affect the
underlying telemetry adapter or wheel force feedback.

Speed and Gear deliberately display `--` because SIMHUB-02B does not yet publish validated
physical speed units or gear encoding. The diagnostic strip says `Telemetry State: Unverified`
because the current contract has no receiver-confirmed validity property.

The dashboard uses only standard Windows Segoe UI and contains no proprietary fonts, logos,
vehicle artwork, or manufacturer artwork.
