# Windows hardware validation

1. Start the utility with the wheel, pedals, shifter, and any button boxes connected.
2. Select **Initialize Devices** and confirm startup itself produces no motor force.
3. Open **Devices → Discovery Details**, run all backend tests, and leave the utility focused until all four SDL sessions finish.
4. Confirm any delayed T300RS arrival appears under the specific backend that observed it.
5. Use **Input Test** to move every axis, button, and POV on each device.
6. Create diagnostic-only assignments under **Multi-Input**, save them, restart, and confirm they reload. Disconnect and reconnect one device and confirm its missing/reconnected state.
7. Complete **Quick Setup**, including explicit native FFB device selection.
8. On **FFB Test**, verify motor output remains off until authorization. Begin at 20%, hold **Run Test**, release it, and verify output stops immediately. Repeat focus-loss and device-disconnect shutdown checks at the lowest practical wheelbase hardware gain.
9. Select **STOP** after each test and export a report. Confirm both report files exist and accurately distinguish completed, unavailable, and untested checks.
