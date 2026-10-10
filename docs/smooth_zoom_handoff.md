# Smooth zoom development handoff

## Latest correction — 10 October 2026

The user clarified that parabola navigation must zoom and pan simultaneously.
`ViewportFlight` now shares quintic progress between position and one quadratic
Bezier in log zoom, with an exact bounded peak even for unequal endpoint zooms.
There is no separate zoom-out/pan/zoom-in sequence or native-mip
travel plateau. Previous native-pan performance figures describe the superseded
path; measure fractional full-redraw flights again before assessing performance.
Pan speed and zoom timing still contribute to total duration, with bounded
pull-back, short-trip handling and exact arrival retained.

The wheel easing tail now finishes when remaining scale displacement and the
nominal frame displacement are at most one pixel across the viewport span.
Final area filtering starts earlier (16 pixels remaining / 8 pixels nominal
movement). This targets rounded-edge changes after apparent rest. Continuously
moving follow cameras use fast sampling; stopping or pausing changes sampling
state and immediately dirties the viewport, even if its origin is unchanged.
The Release build passes 48,327 assertions in 123 test cases, including overlap
of pan and zoom on both sides of the apex. Further visual settling assessment
is required; do not claim all temporal artefacts are eliminated.

Overnight fixture coverage includes deliberate foundations and a steep slope,
all four base rail technologies, bridges, tunnels, waypoints, docks, canal and
lock. Reproducible NoAI scripts are under `docs/validation`. The separate vehicle
world has bus, truck, four locomotive types, ship, aircraft and helicopter;
positions and nonzero speeds were verified. Bus and helicopter picking,
independent vehicle viewport zoom, main-camera following and following cancelling
a flight passed. Remaining vehicle picking/trail comparisons and NewGRF/tram
coverage remain open. No available base-game tram road type was found.

Saved worlds and capture tools are in ignored `build-smoothzoom`. In particular,
`zoom-static-2050-complete.sav` has infrastructure without vehicles and
`zoom-vehicles-fast-follow.sav` has the moving fleet. The prior follow optimization
measured about 60 drawing FPS at fractional zoom without capture; that is a
single-scene result, not a guarantee.

## Git workflow — 10 October 2026

The user superseded the earlier no-commit instruction and authorized checkpoint
and regular development commits. Keep `origin` pointed at the private repository
`https://github.com/Deku-shrub/OpenTTD.git`, with the official
`https://github.com/OpenTTD/OpenTTD.git` as `upstream`. The user authorized
`master` as the private repository's default development branch provided the
build remains stable. Local `master` tracks `origin/master`; use `upstream/master`
to inspect official changes. Promote code to `master` after a successful Release
build, relevant tests and runtime checks; keep unvalidated experiments on feature
branches. The promoted prototype passed 48,600 assertions in 124 test cases and
representative runtime navigation checks. Open visual and coverage issues remain
tracked in the TODO list. `feature/smooth-zoom` retains the initial checkpoints,
and `main` retains the original upstream base revision.
Initial checkpoints: `42d6302fea` (code) and
`0e02fa7152` (guide, recordings and validation fixtures).
The user explicitly chose to retain a private Git repository with shared upstream
history rather than a public GitHub-native fork. Do not change its visibility.
Make focused commits after validation; avoid rebasing published work. Build
outputs, isolated configurations, saved games and runtime logs stay ignored.

## Per-target flight controls

`gui.fly_navigation` is the master switch; `gui.fly_towns` now jointly
controls towns and uncategorised map destinations. This combined option defaults
on beneath the disabled master switch; additional
categories default off. `ViewportFlightTarget` and `IsViewportFlightEnabled`
carry typed permissions through viewport navigation. Active flights retain
their category and cancel if that category or the master is disabled.

Additional controls cover vehicles, stations, industries, news/events, signs,
smallmap, company headquarters, depots, waypoints, order destinations, scenario
links (stories/goals/league tables), subsidies, game-script locations and other
locations. News references use News even for a vehicle; order destinations use
Order even for a station. Generic Other does not override disabled categories.
The former `fly_other_navigation` setting has been removed. Uncategorized map
locations and console commands now share `fly_towns`; all other category switches
remain independent. Existing `fly_towns` configurations retain their value. Explicit instant navigation,
Ctrl-click extra viewports and continuous vehicle following remain immediate.
Vehicle location flights target the selected position, not a moving endpoint.

Independent permissions have a full one-enabled-category matrix regression
using real GUISettings fields. The Release build passes 48,600 assertions in
124 cases. Runtime town/station/industry/vehicle controls passed independent
on/off checks. Disabling an active station category left its land crop
pixel-identical nine seconds later, past the planned eight-second duration.
Vehicle following interrupted a vehicle-target flight. The Expert settings UI
displays each toggle's On/Off state; its screenshot is in the guide. News-item
and remaining secondary-control runtime checks are still open.

## Scope and decisions

Implement per-sprite fractional zoom for OpenTTD. Start by verifying sea and
land, then continue with foundations, buildings, bridges and moving vehicles.
The user authorized sustained development, dependency setup and Windows desktop
automation. The initial work was uncommitted; the current Git workflow above
supersedes that constraint.

Smooth wheel zoom is an opt-in client setting (`gui.smooth_zoom`), disabled by
default. Preserve existing integer rendering and savegame compatibility. A
moderate performance cost is acceptable; avoid large persistent sprite caches
or costs that would make upstream adoption impractical.

## Implementation in the working tree

- `Blitter::DrawScaled` and `SupportsSmoothZoom` provide an explicit capability
  boundary. Unsupported blitters keep integer zoom.
- The reference `32bpp-simple`, optimized RLE, SSE, software animation, and
  OpenGL `40bpp-anim` paths support fractional sprites. Default blitters reuse
  their existing integer mipmaps and a bounded reusable scanline scratch buffer.
- Normal mip selection keeps the sampling footprint below two source pixels
  per axis (at most nine taps). Unaligned sub-sprite clipping selects a finer
  available mip to prevent excluded colours bleeding into foundations.
- Remapping and brightness are resolved before filtering. Transparent sprite
  edges use centre coverage to avoid dark seams between independently composited
  opaque terrain sprites. This favours crisp silhouettes over edge antialiasing.
- Fractional `DrawPixelInfo` carries an exact virtual origin and actual output
  dimensions, independently of its conservative integer culling rectangle.
- Viewport picking, redraw bounds, developer overlays, labels, scrolling and
  resize account for the fractional scale. Fractional scrolling redraws rather
  than copying pixels with an obsolete sampling phase.
- Wheel controls use four steps per octave, anchor the cursor in ordinary
  viewports and preserve the centre for following/vehicle viewports. Buttons
  select the next integer level in their direction. Integer levels canonicalize
  to the existing fast path. See [smooth_zoom_controls.md](smooth_zoom_controls.md).
- Savegames retain integer zoom and preserve the viewed centre when rounding.
  Loading resets fractional state. Runtime switching to an unsupported blitter
  also returns existing viewports to integer zoom.
- Fractional sprites report animated palette dependencies from their contributing
  samples. A 64×8 screen-block tracker redraws affected blocks when palette entries
  change. Partial repaints retain dependencies conservatively.
- Wheel targets use critically damped log-scale motion in real time, preserving
  velocity across repeated input and reversal. The compact toolbar percentage
  shows current magnification; its tooltip gives the nearest traditional level.
  Clicking cancels animation and snaps to that level.
- Active wheel motion uses centre sampling to reduce rendering cost. Area
  filtering returns during the last visible movement; snapping or cancelling
  also restores it and requests a redraw. Both
  modes use the same clipping and palette/animation-aware pixel writers, with
  bounded scratch memory and no persistent decoded-sprite cache.

## Verification status

For a reviewable checklist, see [smooth_zoom_todo.md](smooth_zoom_todo.md).
The earlier prototype passed 3,605 smooth-zoom assertions and produced aligned
fractional screenshots of coastlines, terrain and buildings. The sampler was
subsequently measured roughly four times faster than the initial implementation.
The earlier Release build includes 160 ms animated controls, a toolbar snap
readout and palette dependency tracking. The complete suite passed 7,238
assertions in 110 cases; the separate benchmark passed 60 assertions. Runtime
checks confirmed intermediate scales, rapid four-notch targets and reversal,
4x/0.125x limits, snapping 1.3x to 1x and cancelling an active transition.
The readout was inspected at 640×480 and ordinary widths. Remaining dynamic
sprite and interaction checks are explicitly unchecked in the checklist.

The earlier sampler benchmark measured about 41–57 ms per destination megapixel
for the tested fractional cases; this is not an end-to-end frame-time measure.
The Windows desktop was locked: use the helper's `-PrintWindow` capture option
to inspect the test game's own window surface. The helper now uses physical
DPI coordinates. Foreground subjective smoothness remains to be assessed.

The latest active-motion profile exposed a rendering bottleneck: one transition
produced only three updates in 305 ms, with a 157 ms maximum gap. Idle near-60 FPS
does not establish smooth zoom. The compact UI, velocity-preserving controller
and motion sampler are now in the rebuilt executable. The final complete suite
passed 10,035 assertions in 114 cases; the sampler benchmark passed 96 assertions.
The motion sampler is about 3–4 times faster than area filtering. Ordinary town
checks average 28–36 ms between updates; a coastal four-notch run produced 21
updates in 573 ms with a 35 ms maximum gap. An extreme zoom to 0.125x still had a
99 ms maximum gap. This is a better candidate, not proof of fully smooth motion.
The compact readout was inspected at 1280x800 and 640x480, live snap cancellation
and both limits passed, and captured frames confirm settled filtering returns.
See the current-work checklist and ignored `benchmark-fast-boundary.txt`,
`runtime-fast-final.log`, `runtime-fast-boundary.log`, `motion-final-*.png`.

Later feedback identified a delayed second texture stabilization after apparent
rest. Filtering had waited for the almost invisible analytic easing tail. The
latest executable restores area filtering when remaining motion is within eight
screen pixels and estimated movement per 30 ms frame is within four pixels;
it settles exactly once the remaining displacement and velocity are each below
a quarter-pixel threshold. The velocity condition handles crossing a new target
with reversal momentum. Both thresholds scale with the viewport's longest side.
The complete suite passed 10,084 assertions / 116 cases, including quality-tail
and momentum regressions. A capture-free 960x600 check settled in 348 ms with a
48 ms maximum interval. User-guide animations were refreshed for this change.
Small sampler texture changes during final motion remain possible; foreground
confirmation of the delayed-rest fix remains open. Evidence:
`build-smoothzoom/validation-settle-full.txt` and `demo-*-timing.json`.

`src/tests/smooth_zoom.cpp` covers filtering, pitch guards, clipping, opaque
seams, remapping, partial alpha, redraw partition invariance, and optimized/SSE
encoded mipmaps. `smooth_zoom_controls.cpp` tests production numerical helpers
for wheel steps, limits, cursor anchoring and button targets. A hidden timing
case in `smooth_zoom_benchmark.cpp` runs only when explicitly selected.

Known risks to inspect visually: integer mip artwork changes at level boundaries;
transparent/partial-alpha sprite silhouettes; animated sea; source subrectangles
on slopes/foundations; dirty partitions under overlapping windows; labels;
vehicle picking; resizing; save/load; and 8bpp fallback.

## This Windows environment

### Fly-to navigation

The latest request adds opt-in parabolic zoom-and-pan destination navigation.
Source implementation uses `ViewportFlight` in `src/core/viewport_flight.hpp`
and a transient per-window map in `main_gui.cpp`. It is independent of the
wheel smooth-zoom flag but requires a supported blitter. Town directory/location
buttons call `ScrollMainWindowToTown`; other `ScrollWindowTo` callers supply an
independently enabled navigation category. Explicit instant jumps bypass it.
Defaults: master off, towns on beneath the master, additional destinations off,
pan 100% viewport/second, zoom
timing 350 ms per end, two extra zoom-out levels, automatic traditional town fit.
Town fitting uses the cached zone radius and building-height allowance; it is
approximate. Other destinations preserve departure zoom. No persistent caches
or savegame fields are added. See the README and current-work checklist.

Flights share eased progress between centre interpolation and a quadratic
parabola in logarithmic zoom. Position and zoom change simultaneously. Endpoint
scales, extra pull-back and client zoom limits bound the curve. Nearby same-scale
trips retain their zoom. The pan contribution to total duration is bounded at
150–4,000 ms. New destination requests restart
from the displayed centre and scale without a position jump, preserving the
intended landing zoom for general destinations; velocity continuity
on flight retargeting is not implemented. Manual scrolling, wheel/buttons/snap,
Escape, load, close and feature disable cancel flights. Changing settings affects
the next flight. The complete suite passed 48,206 assertions in 121 cases.
Runtime checks confirmed paused town-list flights with wheel zoom disabled,
automatic 100% and fixed 50% arrivals, general destinations retaining arrival
zoom, smallmap navigation, Escape/wheel/feature-disable interruption, instant
jumps, and 8bpp legacy fallback. Flags and timing values were inspected in
Game Options → Advanced, Expert category. Generic retargeting now retains the
original 100% landing zoom rather than the temporary 25% travel view; runtime
rechecked successfully. Changing viewport zoom limits cancels the captured
flight, and in-flight loading of the isolated save restored its native 100%
view with no old-flight completion. The final full suite still passes 48,206
assertions in 121 cases. See the remaining foreground runtime checklist.
The user guide contains the real town-flight recording made on 10 October 2026.
Evidence: `validation-flight-full.txt`, `runtime-flight.log`,
`runtime-flight-stderr.log`, `runtime-flight-disable.log`,
`runtime-flight-wheel.log`, `runtime-flight-8bpp.log`,
`runtime-flight-retarget-final.log`, `runtime-flight-lifecycle.log`,
`runtime-flight-lifecycle-stderr.log`, and `flight-after-load.png`.

Automation notes: Windows direction keys are polled with `GetAsyncKeyState`;
posted arrow messages do not validate physical arrow scrolling on the locked
desktop. Leave that foreground check open. The console's `load` command uses
file-browser names (`load zoom-validation.sav` here), not full paths. Live load
resets the UI and can leave the console helper's visibility assumptions wrong;
inspect the window rather than assuming subsequent commands ran. A focused
settings filter can also intercept console automation. If necessary, close only
the verified isolated PID, checking both executable and config command line.

### User documentation recordings

The user requested periodic animated demonstrations of fast/slow wheel input,
small/large changes, and the modified zoom controls. Keep these in the prototype
user README so they can carry forward into the end-user documentation. Refresh
them after meaningful motion/rendering/control milestones and before handing
over an updated prototype; no separate approval is needed for these captures.

The current five lossless animated WebP files are in `docs/images/smooth-zoom`.
They are actual game-window captures from 9 October 2026, roughly 20 FPS with
measured capture timing preserved. Captures add overhead and are not performance
benchmarks. The controls clip shows a small zoom-out and clicking the percentage
to snap back to 100%. Keep the scene paused and return each clip to its starting
scale so loops remain easy to follow. Update the README recording date/captions.

The local repeatable helper is ignored `build-smoothzoom/record-demos.py`, using
Pillow installed in ignored `build-smoothzoom/capture-deps`. Launch the isolated
game at 960x600, enable smooth zoom and reset with `smoothzoom 1` before each
scenario (`small`, `fast`, `slow`, `big`, `controls`). The helper checks the PID
and executable identity before posting input or recording. `PrintWindow` with
`PW_CLIENTONLY` (1) captured the game in 7–15 ms here; full-content flags (3)
took about 230 ms and must not be used for these motion demonstrations. Capture
the game window only. Raw timing JSON and still-frame checks stay in the ignored
directory; use the committed-path documentation assets for the final guide.

- Repository: `C:\Users\chris\github\OpenTTD`.
- Existing build: `C:\ottd-build`, Visual Studio 2022 x64 Release.
- CMake: `C:\Program Files\CMake\bin\cmake.exe`.
- Dependencies: `C:\vcpkg`, static x64 triplet; required packages already installed.
- With limited available memory, use one compiler worker: `$env:_CL_ = '/MP1'`, then
  `cmake --build C:\ottd-build --config Release --target openttd openttd_test -- /m:1`.
  The earlier unconstrained build exhausted available resources and was paused.
- Installed dependencies are sufficient; the current build is configured with
  `VCPKG_MANIFEST_INSTALL=OFF` to avoid repeating dependency installation.
- Shell tools have sometimes started in `C:\` despite a requested working
  directory. Use absolute paths or explicitly set the location. Restricted
  sandbox directory access may need an approved escalation.

Ignored `build-smoothzoom` contains isolated config, extracted OpenGFX tar, test
scripts and artifacts. `desktop.ps1` launches the game and drives its console,
wheel, clicks and resize. Keep the launch shell alive while testing; ending its
executor job can kill the game. `popups.ps1` captures the desktop and enumerates
visible dialog text. Its close action verifies identity and is restricted to the
test executable. Do not dismiss system approval prompts or unrelated windows.

The isolated AI fixture is `build-smoothzoom/ai/ZoomValidation`. Run `rescan_ai`,
`start_ai ZoomValidation`, then unpause. It builds a short bus route and optionally
a bridge, logs a `scrollto` tile and places a centre sign. Read its log for failed
construction steps rather than assuming every structure was built.

The console command `smoothzoom 1.3` selects 1.3x normal magnification; `0` snaps
back to integer. `screenshot viewport no_con <name>` captures the actual viewport
and therefore fractional drawing. Normal/big/world screenshots use temporary
integer viewports. Send screenshots on separate rendered frames rather than
batching several scale changes and asynchronous screenshots into one script.
