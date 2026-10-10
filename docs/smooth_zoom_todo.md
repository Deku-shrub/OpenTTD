# Smooth zoom progress

Updated 10 October 2026. Checkpoint and regular commits are now authorized.
A checked implementation item
does not imply that every runtime check has passed or that the installed
prototype executable contains the latest source changes.

## Implemented

- [x] Fractional sprite rendering for the main 32bpp blitter families.
- [x] Reuse existing mipmaps with bounded scanline scratch memory.
- [x] Palette/company remapping, brightness and partial-alpha filtering.
- [x] Clipping and transparent-edge handling to avoid opaque terrain seams.
- [x] Fractional viewport coordinates, redraw bounds and picking conversions.
- [x] Opt-in client setting, disabled by default; integer zoom fast path retained.
- [x] Quarter-octave wheel targets, client zoom limits and cursor anchoring.
- [x] Zoom buttons select adjacent traditional integer levels.
- [x] Labels, scrolling, resizing and extra/vehicle viewport scale conversions.
- [x] Savegame integer zoom format retained; loading resets fractional state.
- [x] Unsupported blitters fall back to integer zoom.
- [x] Prototype setup README and corrected settings location.
- [x] User-guide animated examples for fast/slow input, small/large changes,
      and the compact percentage/snap control.
- [x] Animated wheel transitions on real-time UI updates, preserving velocity
      when repeated input changes the target.
- [x] Toolbar current magnification / nearest traditional level readout and snap control.
- [x] Animated palette dependency tracking by screen block.

## Current work: animated controls and toolbar

The first animated prototype was reported as still feeling stepped. The next
pass is in progress; do not treat the earlier control checks below as proof of
subjective smoothness.

- [x] Replace restarted easing curves in source with velocity-preserving,
      critically damped zoom motion.
- [x] Add production motion tests for retargeting, reversal, frame partitions
      and exact settling.
- [x] Implement a compact percentage-only toolbar button; nearest traditional
      level moves into its tooltip and clicking still snaps.
- [x] Build and validate the new motion and compact UI.
- [x] Benchmark the renderer's new row-pointer/tap specialization optimization.
- [x] Profile active zoom frame intervals, including maximum gaps.
- [x] Active profiling exposed the bottleneck: a no-capture transition took
      3 updates / 305 ms, with a 157 ms maximum interval despite near-60 idle FPS.
- [x] Implement a fast centre-sampling path for active wheel motion; settled
      views keep area filtering. No persistent decoded-sprite cache added.
- [x] Validate fast motion sampling, quality restoration and live frame pacing.
- [x] Tune acceleration to reach about 90% of a notch in 150 ms; preserve
      velocity across repeated input and use exact traditional endpoints.
- [x] Address delayed texture stabilization: restore area filtering during the
      final visible movement, then finish the easing tail at a quarter-pixel
      screen-space tolerance. Momentum is checked before changing quality.
- [x] Regression checks at 640, 1280 and 3840 pixels, plus reversal momentum;
      complete rebuilt suite: 10,084 assertions / 116 cases. Capture-free 960x600
      runtime check settled in 348 ms, maximum frame interval 48 ms.
- [x] Final Release build: 10,035 assertions / 114 cases; separate sampler
      benchmark: 96 assertions / one case. Both motion and settled samplers cover
      clipping, remapping, alpha, mipmaps and dirty-partition consistency.
- [x] Compact percentage control inspected at 1280x800 and 640x480; clicking
      during motion cancels at an exact traditional level. Both zoom limits pass.
- [ ] Reduce remaining frame gaps in dense, widely zoomed-out scenes. Ordinary
      town checks average 28–36 ms between updates; an extreme multi-notch zoom
      to 0.125x averaged 44 ms with a 99 ms maximum gap on this machine.
- [ ] Assess perceived smoothness in an unlocked foreground game.
- [ ] User confirmation that the delayed second stabilization is gone; a small
      texture change during the final motion can still occur between samplers.

- [x] Build and validate animated wheel transitions, including while paused.
- [x] Verify repeated wheel input updates the target, reversal and limits.
- [x] Cursor-anchor numerical tests; intermediate and settled in-game frames captured.
- [ ] Verify cursor anchoring while dragging, edge scrolling or following vehicles.
- [x] Build and validate toolbar magnification / nearest traditional level readout.
- [x] Verify clicking the readout stops animation and snaps to that level.
- [x] Check toolbar layout at 640×480 and ordinary window widths.
- [ ] Check larger interface sizes and right-to-left toolbar layout in-game.
- [x] Rebuild the playable executable and document the new controls.

## Current work: fly-to navigation

- [x] Implement bounded parabolic zoom arcs with eased departure, pan and arrival.
- [x] Feature flag off by default; a separate option enables other destinations.
- [x] Town directory, town location button and newly founded town integration.
- [x] Automatic town-size estimate or fixed traditional arrival zoom.
- [x] Configurable flight speed, whole-flight duration scale, curvature and
      maximum extra zoom-out (replacing the initial per-end zoom timing).
- [x] Manual wheel, drag, arrows, edge scrolling, Escape and snap interrupt flights.
- [x] New destinations retarget from the displayed view; instant jumps bypass flights.
- [x] Lifecycle cancellation on close, load, feature disable and vehicle following.
- [x] Add production-path numerical regressions for bounds, phases, timing and fitting.
- [x] Complete rebuild and full suite: 48,206 assertions in 121 cases.
- [x] Town-list automatic 100% arrival, fixed 50% arrival, and repeat-click handling.
- [x] Flag-off navigation, Escape/wheel/feature-disable interruption and instant jumps.
- [x] General destinations, smallmap, disabled wheel setting and 8bpp fallback.
- [x] Flight flags and timing controls inspected in the Advanced settings UI.
- [x] Capture an animated fly-to demonstration and add it to the user guide.
- [x] Revalidate the final retargeting refinement: preserve the intended arrival
      zoom when selecting a different general destination during a flight.
- [x] Changing zoom limits during flight cancels and leaves the view stable.
- [x] Loading the isolated save during flight cancels the trip and restores its
      native 100% view; captured after waiting past the old flight duration.
- [ ] Foreground runtime checks: dragging/arrows and the town location button.
      Posted arrow messages cannot exercise Windows' asynchronous
      physical-key polling on the locked desktop.
- [x] Vehicle-follow interruption: double-clicking the helicopter location control
      cancels an active 6 s flight and its retargeted route, with no delayed landing.
- [x] Cancel flight immediately on drag or wheel-pan gesture start, before
      the first scrolling delta. Runtime right-drag press interrupted an 8 s
      route at 90% zoom; the captured land view was pixel-identical nine seconds
      later, with no resumed flight or jump to its former destination.
      Following now cancels wheel/flight motion immediately in source and build.
- [ ] Assess subjective comfort and fit quality on small and large towns.

## Verified so far

- [x] In-game screenshots at fractional zoom: land, sea, coastlines, roads,
      trees and buildings show aligned placement.
- [x] 3,605 smooth-zoom assertions passed before the latest integration edits.
- [x] Fresh Release build and complete suite: 7,238 assertions in 110 test cases.
- [x] Optional production-blitter benchmark: 60 assertions passed separately.
- [x] Paused runtime checks: one notch reaches 0.8409x; four accumulated notches
      reach 0.5x integer rendering; four reverse notches return to 1x.
- [x] Runtime wheel limits settle at 4x and 0.125x on the integer fast path.
- [x] Snap returns 1.3x to 1x and cancels a live wheel transition.
- [x] Filtering, clipping, pitch guards, seams, remapping, partial alpha,
      dirty-partition consistency and encoded mipmaps covered by tests.
- [x] Numerical control tests cover wheel targets, limits, anchoring and buttons.
- [x] Sampler optimization measured roughly four times faster than the initial
      fractional sampler; fractional rendering remains much slower than integer.

## Rendering and performance still to validate

- [x] Build a separate static-only validation world: rail tracks, a two-platform
      station, depot, path signal, rail bridge and tunnel, road depot, bus stop,
      small airport, dock and ship depot. No vehicles constructed.
- [x] Inspect settled 130% infrastructure scenes and record eight-notch zoom
      out/return demonstrations for rail, tunnel, airport and dock/shoreline.
      Clips are included in the user README and pass distinct-frame checks.
- [x] Record fast and gentle flights along the same route, plus a fixed 50%
      town arrival. Verified actual journeys: 1.08 s, 2.74 s and 0.74 s.
- [x] Validate native-zoom flight pixel scrolling optimization after rebuild;
      Historical measurements below used the superseded staged flight. The
      simultaneous arc redraws at fractional scales; remeasure its performance.
      fractional phases still redraw completely. Town-directory occlusion passes.
      Capture-free long journey: 187 frames / 3.12 s, maximum interval 35 ms;
      native pan averages 16.1 ms, maximum 21 ms. Landing matches instant-jump
      viewport pixels exactly. Overlay views conservatively redraw completely.
- [x] Build and inspect later-year base rail technologies, road bridges/tunnels,
      waypoints, canals and a lock. Three additional animations are in the guide.
      Test-world save: `zoom-static-2050.sav`; reproducible NoAI fixture under
      `docs/validation/ZoomStatic` chooses an airport available in the current year.
- [x] Build and inspect deliberately constructed road and rail foundations
      and a steep slope (slope mask 29); zoom animation included in the guide.
- [ ] Tram/NewGRF coverage: the base-game fixture reports no available tram
      road type. Verify supported NewGRF infrastructure separately.

- [x] Integrate and unit-test palette dependency tracking, including off-centre
      samples, remapping, invisible taps and partial screen-block repaints.
- [ ] Animated sea: colour cycling, seams, scrolling and overlapping windows.
- [ ] Moving vehicles: animation frames, redraw trails, company colours,
      shadows, click targets and following viewports.
      A separate 2050 world now has nine verified moving vehicles: bus, truck,
      trains on four base rail technologies, ship, aircraft and helicopter.
      Exact picking/follow checks and redraw comparisons are in progress.
- [x] Bus picking at 330% and helicopter picking at 130%; independent fractional
      vehicle viewport zoom, and main follow-camera wheel zoom while moving.
- [x] Validate motion sampling for continuously moving follow cameras after rebuild;
      paused views restore area filtering without a settling timer. A bus-follow
      recording was refreshed; a paused land crop matches an exposed-area redraw.
      Four-notch updates measured 22–24 ms average intervals, maximum 37–39 ms,
      during capture. This is representative coverage, not every vehicle state.
- [ ] Other dynamic sprites: animated industries, buildings and effects.
- [ ] Bridges, foundations, steep slopes and clipped source sprites in-game.
- [ ] Partial-alpha silhouettes and mip artwork changes during animated zoom.
- [ ] Labels and signs at intermediate scales, including hit testing.
- [ ] Window occlusion, viewport edges, resize and extra viewports in-game.
- [ ] Save/load centre preservation and runtime blitter switching in-game.
- [ ] OpenGL / 40bpp runtime validation and 8bpp fallback.
- [ ] End-to-end frame-time and resource measurements on land, coast and
      vehicle-heavy scenes; assess suitability for upstream adoption.
- [x] Fresh complete build and full test suite after current integration.
      Latest simultaneous-flight and shorter-settling build: 48,327 assertions
      in 123 cases. Pan and zoom overlap on both sides of the apex, including
      unequal endpoint zooms without extra pull-back.
- [x] Replace staged navigation with simultaneous parabolic zoom and pan.
- [x] Shorten the subpixel wheel tail and restore final filtering earlier.
- [ ] Confirm the user's reported after-rest artefacts are eliminated across
      dense scenes and native mip transitions. Paused land crop recordings stay
      unchanged after their final movement; that does not prove every scene.

Dynamic sprites already reach the fractional renderer; the remaining work is
to verify their behavior and address failures, rather than to assume their
animation and interaction are finished.

## Environment and delivery

- [ ] Periodically refresh the user-guide animations after meaningful changes
      to motion, rendering or controls, and before handing over a new prototype.
      Keep example captions and the recording date aligned with the build.

- [x] Sufficient disk space available for the current build and validation.
- [x] Update the development handoff with current validation results.
- [x] Preserve the prototype uncommitted until the user authorizes checkpoints.
      Superseded on 10 October: commit and push to the private origin, retaining
      the official repository as upstream. Do not change repository visibility.
- [x] Add independent fly-target toggles for towns, vehicles, stations,
      industries, news/events and the remaining navigation entry points.
      Fifteen destination categories, gated by the master switch; towns and
      uncategorised map destinations now share one option. All other options
      default off; the shared option cannot override a disabled category.
- [x] Rebuild and test permission isolation: 48,600 assertions in 124 cases.
- [x] Inspect per-target On/Off settings in the Expert UI; screenshot in guide.
- [x] Runtime town, station, industry and vehicle category checks, including
      category off while Other is on, and category on while Other is off.
      Disabling an active station category preserves the view past its former
      duration; vehicle following interrupts a vehicle-target flight.
- [x] Merge town and uncategorised map flights into one option, retaining
      `gui.fly_towns`; other destination switches remain independent. Release
      build and 48,600 assertions pass; shared-map on/off, mid-flight cancellation
      and town-directory flight checked. Guide screenshot refreshed.
- [ ] Runtime news-item clicks and remaining secondary navigation controls.

User instructions: [prototype setup](smooth_zoom_readme.md).
Control details: [smooth zoom controls](smooth_zoom_controls.md).

Local evidence is in ignored `build-smoothzoom`: `validation-controls-full.txt`,
`runtime-controls-2.log`, `runtime-controls-3.log`, `runtime-snap.log`,
`runtime-limits.log`, `runtime-cancel.log`, `rapid-wheel-*.png`,
`controls-narrow.png` and `benchmark-palette-tracking.txt`. The Windows desktop
was locked during these checks; game-window surface captures and in-game
screenshots were used. The automation helper's DPI coordinates were corrected.
Smoothness still needs subjective assessment on an unlocked, foreground window.

Latest evidence: `validation-fast-motion-full.txt`, `benchmark-fast-boundary.txt`,
`runtime-fast-final.log`, `runtime-fast-boundary.log`, `compact-fast-narrow.png`,
`compact-fast-coast.png` and `motion-final-*.png`. Before the motion sampler a
single transition had three updates in 305 ms (maximum gap 157 ms). The final
terrain capture run had 12 updates in 398 ms (maximum gap 36 ms); the capture-free
coastal four-notch run had 21 updates in 573 ms (maximum gap 35 ms). Moving-image
centre sampling is about 3–4 times faster than settled area filtering in the
synthetic benchmark. Captures confirm filtering returns after settling; they
cannot establish subjective smoothness. No game error dialogs were found.

## Perceptual flight update

- [x] Replace distance-based timing with van Wijk-Nuij perceptual path length.
- [x] Derive height from distance and configurable curvature; flatten to zoom limits.
- [x] Avoid extra pull-back within one starting viewport width.
- [x] Bound animated duration to 300-2200 ms; cut if unbounded duration exceeds 2500 ms.
- [x] Preserve fractional arrival zoom for ordinary destinations.
- [x] Respect smooth scrolling and cancel on any actual key press.
- [x] Blend vehicle location flights toward the live position in the final 20%.
- [x] Standalone numerical regression: 3,273 assertions in 13 cases, including the published d3 example.
- [x] Full Release rebuild and integrated tests: 13,627 assertions in 130 cases.
- [x] Runtime 305 ms short hop with native pan, approximately 1.47 s default
      town arc, 0.89 s fast arc, fixed 50% arrival and immediate long-trip cut.
      Smooth-scroll off prevented flights; key/wheel takeover cancelled them.
      Keyboard-cancelled land remained pixel-identical beyond planned arrival.
- [x] Refresh four real town-flight recordings and capture pacing controls;
      no recorded land-crop changes after arrival in these paused fixtures.
- [x] Moving bus flight completed in 1.22 s and retained 130% arrival zoom.
- [ ] Runtime moving vehicle news references and all other vehicle approaches.
- [x] Generate an isolated 2048-square map and verify the (10,10) to
      (2000,2000) journey arrives immediately without an active flight.
- [ ] Subjective tuning on a 2048-square map.

## Retrofit and compatibility

- [x] Document existing-save retrofit, local-only options, integer camera save/load,
      rollback version limits, renderer requirements and mod-type expectations.
- [ ] Stock-created save -> patched save -> matching stock build round trip.
- [ ] Representative 8-bit and 32-bit / extra-zoom NewGRF runtime coverage,
      including custom infrastructure, animated sprites, articulated vehicles and trams.
- [ ] Multiplayer with matching patched revisions and per-client camera settings.
- [ ] Classify and validate multiplayer Game Script camera redirects; currently
      these use the combined town/map category rather than the script category.
- [ ] Validate any chosen engine patch pack after source integration.
