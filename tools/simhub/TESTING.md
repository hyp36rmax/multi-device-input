# SIMHUB-02A Synthetic Sender

This development utility transmits deterministic **synthetic test data**, not live OutRun 2006 telemetry. It only sends UDP packets to `127.0.0.1:30777` and does not interact with the game or physical hardware.

## Receiver test

1. Register `OutRun 2006 C2C Multi Input.simdef` in SimHub and open its Telemetry Receiver Tester.
2. Confirm the receiver is listening on UDP port `30777`.
3. Run `SimHub-OutRun2006-SyntheticSender.exe` from this package.
4. Confirm Total packets and Valid packets increase at approximately 60 Hz.
5. Confirm Speed cycles between 0 and 120 km/h, Gear cycles through N and 1–5, and Steering sweeps between -1 and +1.
6. Confirm Car ID is 27, Car Name is `Ferrari F430 Spider (OutRun)`, Stage ID is 0, Road Activity changes, and Impact Intensity produces short pulses.
7. Press Ctrl+C in the sender console and confirm it reports a clean stop.

Passing the repository's synthetic receiver test confirms the binary contract and localhost transport. End-to-end compatibility is confirmed only after SimHub reports the packets as valid.
