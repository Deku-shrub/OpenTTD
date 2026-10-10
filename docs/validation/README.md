# Smooth zoom validation fixtures

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
