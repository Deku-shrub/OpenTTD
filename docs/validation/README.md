# Smooth zoom validation fixtures

## Quick navigation demo

Run `launch-prototype.ps1` with a compiled Windows game and installed OpenGFX
(the default executable is `C:\ottd-build\Release\openttd.exe`). It loads
`zoom-navigation.sav`, the existing fleet fixture including stations and all
base vehicle categories. The paused world is ready for directory and location
button tests; unpause for live vehicle approaches. Select the fixture company
in toolbar station/vehicle menus rather than the empty human company.

The launcher enables the main flight switch and smooth zoom in a separate
`navigation-demo.cfg`. Use `-NewMap` for a fresh map or `-Savegame` for another
world. The NoAI scripts below are copied into the isolated runtime directory so
loading does not require rebuilding the fixture. Keep the terminal open.

## Rebuilding the fixtures

`ZoomStatic` is a NoAI fixture for a disposable test game. It builds infrastructure
without constructing vehicles. Copy its directory into that game's `ai` search
directory, then run `rescan_ai`, `debug_level script=5`, and `start_ai ZoomStatic`
in the console. Unpause while it builds; pause before comparing rendering frames.
Use a temperate 256×256 map with enough flat land and some hills and water.
Starting in 2050 makes the base game's later rail technologies available.

Read `STATIC READY`, `STATIC FAILED` and completion messages in the AI debug
window or script debug log. Each successful structure receives a sign and a
console `scrollto` tile. Failed builds do not establish rendering coverage.
Save the completed world for repeated blitter and zoom checks. Loading a saved
fixture does not rebuild it.

`ZoomStaticExtra` adds deliberately raised road/rail foundations and a steep
slope, and reports whether tram infrastructure is available. Run it only after
the main static fixture has finished.

`ZoomVehicles` constructs a bus, truck, trains on available base rail types,
an aircraft, helicopter and ship, each with a short two-destination route.
Use a separate disposable world after completing static checks. Start in 2050,
enable vehicles never expiring, and set a £2,000,000 initial maximum loan in the
new-game settings so obsolete vehicle types remain available and construction
has enough funds. Successful vehicle starts and periodic positions are logged.
Its save callback logs exact vehicle tiles, useful for paused picking checks.
Saving and loading retains the fleet without rebuilding it.

The isolated local worlds and raw captures live under ignored `build-smoothzoom`.
The checked-in demonstration animations are linked from
[the user guide](../smooth_zoom_readme.md). They preserve captured timing and
are demonstrations, not capture-free frame-time measurements.

## Latest checks, 10 October 2026

Final Release build: 13,700 assertions in 132 cases passed. GUI checks covered
Basic master visibility, default-on Expert categories, bus centre flight and
vehicle opt-out, consecutive town navigation retaining 130%, and stable land
when a moving fractional follow camera was paused. At 130% in the 960x600
32bpp-anim / win32:no_threads fixture, captured pan update intervals improved
from 109 ms median to 32 ms. Baseline compilation load was present; this is an
observational comparison. No land-crop changes occurred beyond 300 ms after
release. Raw timings/captures are in ignored build-smoothzoom/pan-*.json and
followed-paused-*.png. Larger-resolution and NewGRF performance checks remain open.
