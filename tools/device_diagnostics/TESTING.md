# Windows hardware validation

1. Start the utility with the wheel, pedals, shifter, and any button boxes connected.
2. Select **Initialize Devices** and confirm startup itself produces no motor force.
3. Open **Devices → Discovery Details**, run all backend tests, and leave the utility focused until all four SDL sessions finish.
4. Confirm any delayed T300RS arrival appears under the specific backend that observed it.
5. On a Fanatec DD2, record every SDL and DirectInput interface shown. Verify the labels remain distinct, then use **Input Test** to identify which interface reports steering movement.
6. Confirm initialization offers **Start Quick Setup** and **Not Now** without capturing. Complete all eight Quick Setup assignments using the six-second windows. Exercise Retry, Skip, Back, and Cancel, then combine a wheel, separate pedals, shifter, button box, and FFB endpoint in one profile. Restart and confirm the profile reloads.
7. Use **Input Test** to move every axis, button, and POV on every interface. Note whether each is detected-but-inactive or responsive.
8. On **FFB Test**, verify the sole FFB endpoint is selected automatically, or that ambiguous endpoints require confirmation. Verify motor output remains off until authorization. Begin at 20%, hold **Test Left** and **Test Right**, release each, and verify output stops immediately. Verify physical direction; do not assume the label is correct for every wheelbase.
9. Run Re-detect and confirm the previous FFB identity is restored, acquisition reports Ready, and no torque is generated. Exercise native spring, damper, and periodic effects plus one labeled synthetic effect. Repeat focus-loss, timeout, emergency STOP, and device-disconnect shutdown checks at the lowest practical wheelbase hardware gain.
10. Select **STOP** after each test, export a report, open the Exports folder, and preserve both identically named files.
