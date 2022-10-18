# Braun Series 8 Shaver Dock IOT... for some reason
`¯\_(ツ)_/¯`

It's just an experiment so far. Use PlatformIO to build. Supports Braun 5430 dock. Requires manual soldering to traces on the control board. If you also want to try this, for some reason, send me an email and I'll update this with instructions.

Supported features so far:
- [x] Press the button (gets current status through LED signals, and can start the shaver clean cycle)
- [x] Read manual button presses of the physical button. Pass them through as usual.
- [ ] In progress: automatically start clean cycle on a delay, without pressing button
- [ ] May do: Connect to an WiFi AP?
- [ ] May do: Read low-liquid level indicator... maybe send a notification?
- [ ] May do: Read "liquid drop" 3 indicators on the right?