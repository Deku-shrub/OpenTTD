# Smooth zoom progress

[Fork homepage](../README.md) | [User guide](smooth_zoom_readme.md) |
[CI requirements and results](validation/upstream-readiness.md)

Updated 25 October 2026. Checkpoint and regular commits are now authorized.
Current cross-renderer and graphics-set results are in
[the coverage notes](validation/zoom-coverage.md). The dated sections below
retain historical checkpoints. Later checked items supersede earlier pending
entries for the same behaviour.

Performance and QA remain active. Demo capture resumed on request on 13 October.
The repeatable runs are described in [the performance protocol](validation/performance-qa.md).

- [x] Recreate all 16 classic/enhanced comparison and mixed-scene clips with overlapping zoom inputs. Verify full decoding, hashes and camera traces. Visual selection remains pending. See [capture notes](validation/promotional-captures.md#recreated-set-13-october-2026).

## Upstream maintenance, 25 October 2026

- [x] Merge the 31 upstream commits and verify zero commits behind upstream.
- [x] Fix the MinGW nightly warning and verify the nightly annotation gate passes.
- [x] Fix missing Doxygen entries and pass source and documentation checks.
- [x] Pass all 150 local tests, all eight platform builds and download publication. Verify the four package hashes and clean source manifests.
- [x] Pass CodeQL verification on the refreshed revision.

Details and CI links are in [the upstream readiness notes](validation/upstream-readiness.md#upstream-refresh-25-october-2026).

## Current performance follow-up

- [x] Compare uncovered dense redraws with fixed-zoom controls and the preceding renderer.
- [x] Add whole-sprite sampling fast paths and independent pixel-boundary checks.
- [x] Run 32-cycle moving and paused zoom checks, vehicle following and label, animation, resolution and OpenGL diagnostics.
- [ ] Reduce the remaining fractional full-redraw overhead, including the slower OpenGL path.
- [x] Complete unlocked foreground interaction and two dense-view repetitions. All nine interaction and sixteen dense-view checks passed.
- [ ] Investigate the small continued private-memory growth in the paused repeat-zoom run.

## Current coverage follow-up

- [x] Show the smooth-zoom master switch in Basic settings alongside the flight master, so the default-enabled feature can be turned off without changing the filter.
- [x] Verify the Basic switch in the Windows build: default On, Off snaps 119% to 100%, a classic wheel step reaches 200%, and re-enabling restores a fractional 168% step.
- [x] Remove superseded documentation clips and per-vehicle movies. Keep capture provenance and a small general-purpose gallery.
- [x] Record 16 full-HD comparison and mixed-scene takes from a downloaded working network. Keep footage and individual vehicle stills in ignored local output. See [capture notes](validation/promotional-captures.md).
- [ ] Select and compress current mixed-scene excerpts for the end-user gallery after reviewing the promotional footage.
- [x] Profile low camera cadence in the dense title-game network at 1080p and separate rendering cost from recording overhead. Pixel composition is a major CPU cost. See the [dense-view follow-up](validation/zoom-coverage.md#dense-view-follow-up-12-october-2026).
- [ ] Compare dense zoom-out with feature-disabled rendering at matching scene density. Accept the base game's dense-map cost, but reduce additional cost from fractional rendering. The previous 27-29 camera updates per second are a measurement, not a fixed performance target.
- [ ] Resolve the smoothness failures reported in all ten enhanced promotional scenarios. Repeat zoom-out, town and vehicle flights, fitted city arrival, manual takeover and mixed-network scenes without recording before replacing the footage.
- [x] Batch fractional sprite rows, reuse native SSE colours and skip transparent margins. Add four-pixel opaque and binary-alpha SSE4 composition with exact animation metadata tests.
- [x] Test a bounded resolved-colour cache for animated SSE sprites, including palette refresh, reused encoding addresses and eviction. Wide-view cadence and foreground footage still need review.
- [x] Guard cache accounting against 32-bit storage-size overflow. Make SSE4 tests respect CPU support, cover SSE2 composition and remove camera-test range-loop copy warnings.
- [x] Verify all eight platform jobs and clean package identities on `e761771dd8`. Source and Doxygen checks passed. See the [release checkpoint](validation/zoom-coverage.md#release-checkpoint-12-october-2026).
- [x] Review the final revision's [CodeQL scan](https://github.com/Deku-shrub/OpenTTD/actions/runs/38181538017). It passed with no new alerts relative to the preceding fork scan.
- [x] Resume foreground testing on the unlocked desktop and test guarded desktop-duplication recording. Town and fitted-city flights improved, but recording overhead still needs work before replacing promotional footage.
- [x] Repeat all ten enhanced scenarios without recording on the unlocked desktop. Assert the town and train destinations, fitted arrival scales and manual takeover. Dense rail zoom-out still averages 45.3 ms per update. See the [foreground measurements](validation/promotional-captures.md#foreground-scenario-checks-12-october-2026).
- [x] Record ten fresh full-HD enhanced QA takes with desktop duplication. Keep them separate from the original footage, with source identities, input events, camera traces and hashes. Visual approval and final promotional selection remain pending.
- [x] Avoid background reads for opaque fractional pixels and remove temporary SSE sample-array storage. Repeated dense-view averages improved by 6-8%, with unchanged cache limits and expanded white-colour/remapping coverage.
- [x] Verify all eight platform build jobs and Doxygen checks on `5e98f414a61b`. All four published platform manifests identify the same clean source revision.
- [x] Finish the `5e98f414a61b` CodeQL review. The scan passed with no new alerts relative to the preceding fork scan.
- [ ] Replace stepped capture gestures with overlapping continuous input and record matching feature-disabled comparisons after the dense-view performance work. Keep deliberate individual-notch demonstrations separate.

- [x] Simplify camera state by removing unused movement-quality flags and sharing vehicle arrival fitting.
- [x] Verify all eight CI build platforms and the nightly checks on the published defaults-enabled revision.
- [x] Finish review-branch package validation and publish the first downloadable fork release.
- [x] Verify the corrected clean Linux revision and matching identities across all four published packages. Reject dirty, wrong-source or mismatched identities before publication.
- [x] Confirm the final Doxygen comparison adds no warnings against the upstream base.

- [x] Enable smooth zoom and the flight master by default for this dedicated fork. Retain per-category switches and existing saved configuration values. Earlier opt-in checkpoints below describe historical defaults.

- [x] OpenGFX: eleven renderer, resolution, interface-scale and language configurations.
- [x] OpenGFX2 Classic 0.8: the same eleven configurations.
- [x] OpenGFX2 High Def 0.8: eleven configurations, with repeated runs consolidated.
- [x] Correct GPU palette-backed label background blending.
- [x] Add GPU-format encoded sprite decoding and clipping unit coverage.
- [x] Verify stock-created save -> modified fractional view/save -> stock reload.
- [x] Check Arabic and accented Latin signs with Segoe UI, Japanese signs with Yu Gothic.
- [x] Verify the NewGRF tram fixture is operating and record the dynamic scene with trees, company colours, wires and adjacent trains.
- [x] Capture comparative pan-performance measurements without image-recording overhead.
- [x] Retain fractional pixel phase and reuse pixels while panning: about 60 drawing FPS at 960 × 600 and 58 at 1920 × 1080, up from about 47/16 on the test machine.
- [x] Check copied land/label pixels against full redraws at 130%, 84% and 66⅔%. Move palette dependencies with copied pixels and keep picking on the rendered origin.
- [x] Build and inspect eight objects, including gasometer, transmitter, lighthouse, substations and forests. Check transparency and record zoom/pan demonstrations.

A checked implementation item
does not imply that every runtime check has passed or that the installed
prototype executable contains the latest source changes.

## Implemented

- [x] Fractional sprite rendering for the main 32bpp blitter families.
- [x] Reuse existing mipmaps with bounded scanline scratch memory.
- [x] Palette/company remapping, brightness and partial-alpha filtering.
- [x] Clipping and transparent-edge handling to avoid opaque terrain seams.
- [x] Fractional viewport coordinates, redraw bounds and picking conversions.
- [x] Opt-in client setting, disabled by default. Integer zoom fast path retained.
- [x] Quarter-octave wheel targets, client zoom limits and cursor anchoring.
- [x] Zoom buttons select adjacent traditional integer levels.
- [x] Labels, scrolling, resizing and extra/vehicle viewport scale conversions.
- [x] Savegame integer zoom format retained. Loading resets fractional state.
- [x] Unsupported blitters fall back to integer zoom.
- [x] Prototype setup README and corrected settings location.
- [x] User-guide animated examples for fast/slow input, small/large changes,
      and the compact percentage/snap control.
- [x] Animated wheel transitions on real-time UI updates, preserving velocity
      when repeated input changes the target.
- [x] Toolbar current magnification / nearest traditional level readout and snap control.
- [x] Animated palette dependency tracking by screen block.

## Animated controls and toolbar

The first animated prototype was reported as still feeling stepped. The next
pass is in progress. Do not treat the earlier control checks below as proof of
subjective smoothness.

- [x] Replace restarted easing curves in source with velocity-preserving,
      critically damped zoom motion.
- [x] Add production motion tests for retargeting, reversal, frame partitions
      and exact settling.
- [x] Implement a compact percentage-only toolbar button. Nearest traditional
      level moves into its tooltip and clicking still snaps.
- [x] Build and validate the new motion and compact UI.
- [x] Benchmark the renderer's new row-pointer/tap specialization optimization.
- [x] Profile active zoom frame intervals, including maximum gaps.
- [x] Active profiling exposed the bottleneck: a no-capture transition took
      3 updates / 305 ms, with a 157 ms maximum interval despite near-60 idle FPS.
- [x] Implement a fast centre-sampling path for active wheel motion. Settled
      views keep area filtering. No persistent decoded-sprite cache added.
- [x] Validate fast motion sampling, quality restoration and live frame pacing.
- [x] Tune acceleration to reach about 90% of a notch in 150 ms. Preserve
      velocity across repeated input and use exact traditional endpoints.
- [x] Address delayed texture stabilization: restore area filtering during the
      final visible movement, then finish the easing tail at a quarter-pixel
      screen-space tolerance. Momentum is checked before changing quality.
- [x] Regression checks at 640, 1280 and 3840 pixels, plus reversal momentum.
      Complete rebuilt suite: 10,084 assertions / 116 cases. Capture-free 960x600
      runtime check settled in 348 ms, maximum frame interval 48 ms.
- [x] Final Release build: 10,035 assertions / 114 cases. Separate sampler
      benchmark: 96 assertions / one case. Both motion and settled samplers cover
      clipping, remapping, alpha, mipmaps and dirty-partition consistency.
- [x] Compact percentage control inspected at 1280x800 and 640x480. Clicking
      during motion cancels at an exact traditional level. Both zoom limits pass.
- [ ] Reduce remaining frame gaps in dense, widely zoomed-out scenes. Ordinary
      town checks average 28-36 ms between updates. An extreme multi-notch zoom
      to 0.125x averaged 44 ms with a 99 ms maximum gap on this machine.
- [ ] Assess perceived smoothness in an unlocked foreground game.
- [ ] User confirmation that the delayed second stabilization is gone. A small
      texture change during the final motion can still occur between samplers.

- [x] Build and validate animated wheel transitions, including while paused.
- [x] Verify repeated wheel input updates the target, reversal and limits.
- [x] Cursor-anchor numerical tests. Intermediate and settled in-game frames captured.
- [ ] Verify cursor anchoring while dragging, edge scrolling or following vehicles.
- [x] Build and validate toolbar magnification / nearest traditional level readout.
- [x] Verify clicking the readout stops animation and snaps to that level.
- [x] Check toolbar layout at 640×480 and ordinary window widths.
- [ ] Check larger interface sizes and right-to-left toolbar layout in-game.
- [x] Rebuild the playable executable and document the new controls.

## Current work: fly-to navigation

- [x] Implement bounded parabolic zoom arcs with eased departure, pan and arrival.
- [x] Feature flag off by default. A separate option enables other destinations.
- [x] Town directory, town location button and newly founded town integration.
- [x] Automatic town-size estimate or fixed traditional arrival zoom.
- [x] Configurable flight speed, whole-flight duration scale, curvature and
      maximum extra zoom-out (replacing the initial per-end zoom timing).
- [x] Manual wheel, drag, arrows, edge scrolling, Escape and snap interrupt flights.
- [x] New destinations retarget from the displayed view. Instant jumps bypass flights.
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
      native 100% view. Captured after waiting past the old flight duration.
- [ ] Foreground runtime checks: dragging/arrows and the town location button.
      Posted arrow messages cannot exercise Windows' asynchronous
      physical-key polling on the locked desktop.
- [x] Vehicle-follow interruption: double-clicking the helicopter location control
      cancels an active 6 s flight and its retargeted route, with no delayed landing.
- [x] Cancel flight immediately on drag or wheel-pan gesture start, before
      the first scrolling delta. Runtime right-drag press interrupted an 8 s
      route at 90% zoom. The captured land view was pixel-identical nine seconds
      later, with no resumed flight or jump to its former destination.
      Following now cancels wheel/flight motion immediately in source and build.
- [ ] Assess subjective comfort and fit quality on small and large towns.

## Verified so far

- [x] In-game screenshots at fractional zoom: land, sea, coastlines, roads,
      trees and buildings show aligned placement.
- [x] 3,605 smooth-zoom assertions passed before the latest integration edits.
- [x] Fresh Release build and complete suite: 7,238 assertions in 110 test cases.
- [x] Optional production-blitter benchmark: 60 assertions passed separately.
- [x] Paused runtime checks: one notch reaches 0.8409x. Four accumulated notches
      reach 0.5x integer rendering. Four reverse notches return to 1x.
- [x] Runtime wheel limits settle at 4x and 0.125x on the integer fast path.
- [x] Snap returns 1.3x to 1x and cancels a live wheel transition.
- [x] Filtering, clipping, pitch guards, seams, remapping, partial alpha,
      dirty-partition consistency and encoded mipmaps covered by tests.
- [x] Numerical control tests cover wheel targets, limits, anchoring and buttons.
- [x] Sampler optimization measured roughly four times faster than the initial
      fractional sampler. Fractional rendering remains much slower than integer.

## Rendering and performance still to validate

- [x] Build a separate static-only validation world: rail tracks, a two-platform
      station, depot, path signal, rail bridge and tunnel, road depot, bus stop,
      small airport, dock and ship depot. No vehicles constructed.
- [x] Inspect settled 130% infrastructure scenes and record eight-notch zoom
      out/return demonstrations for rail, tunnel, airport and dock/shoreline.
      Clips are included in the user README and pass distinct-frame checks.
- [x] Record fast and gentle flights along the same route, plus a fixed 50%
      town arrival. Verified actual flights: 1.08 s, 2.74 s and 0.74 s.
- [x] Validate native-zoom flight pixel scrolling optimization after rebuild.
      Historical measurements below used the superseded staged flight. The
      simultaneous arc redraws at fractional scales. Remeasure its performance.
      Fractional phases still redraw completely. Town-directory occlusion passes.
      Capture-free long flight: 187 frames / 3.12 s, maximum interval 35 ms.
      Native pan averages 16.1 ms, maximum 21 ms. Landing matches instant-jump
      viewport pixels exactly. Overlay views conservatively redraw completely.
- [x] Build and inspect later-year base rail technologies, road bridges/tunnels,
      waypoints, canals and a lock. Three additional animations are in the guide.
      Test-world save: `zoom-static-2050.sav`. Reproducible NoAI fixture under
      `docs/validation/ZoomStatic` chooses an airport available in the current year.
- [x] Build and inspect deliberately constructed road and rail foundations
      and a steep slope (slope mask 29). Zoom animation included in the guide.
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
- [x] Bus picking at 330% and helicopter picking at 130%. Independent fractional
      vehicle viewport zoom, and main follow-camera wheel zoom while moving.
- [x] Validate motion sampling for continuously moving follow cameras after rebuild.
      Paused views restore area filtering without a settling timer. A bus-follow
      recording was refreshed. A paused land crop matches an exposed-area redraw.
      Four-notch updates measured 22-24 ms average intervals, maximum 37-39 ms,
      during capture. This is representative coverage, not every vehicle state.
- [ ] Other dynamic sprites: animated industries, buildings and effects.
- [ ] Bridges, foundations, steep slopes and clipped source sprites in-game.
- [ ] Partial-alpha silhouettes and mip artwork changes during animated zoom.
- [ ] Labels and signs at intermediate scales, including hit testing.
- [ ] Window occlusion, viewport edges, resize and extra viewports in-game.
- [ ] Save/load centre preservation and runtime blitter switching in-game.
- [x] OpenGL / 40bpp runtime validation and 8bpp fallback with three base graphics sets.
- [ ] End-to-end frame-time and resource measurements on land, coast and
      vehicle-heavy scenes. Assess suitability for upstream adoption.
- [x] Fresh complete build and full test suite after current integration.
      Latest simultaneous-flight and shorter-settling build: 48,327 assertions
      in 123 cases. Pan and zoom overlap on both sides of the apex, including
      unequal endpoint zooms without extra pull-back.
- [x] Replace staged navigation with simultaneous parabolic zoom and pan.
- [x] Shorten the subpixel wheel tail and restore final filtering earlier.
- [ ] Confirm the user's reported after-rest artefacts are eliminated across
      dense scenes and native mip transitions. Paused land crop recordings stay
      unchanged after their final movement. That does not prove every scene.

Dynamic sprites already reach the fractional renderer. The remaining work is
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
      Fifteen destination categories, gated by the master switch. Towns and
      uncategorised map destinations now share one option. All other options
      default off. The shared option cannot override a disabled category.
- [x] Rebuild and test permission isolation: 48,600 assertions in 124 cases.
- [x] Inspect per-target On/Off settings in the Expert UI. Screenshot in guide.
- [x] Runtime town, station, industry and vehicle category checks, including
      category off while Other is on, and category on while Other is off.
      Disabling an active station category preserves the view past its former
      duration. Vehicle following interrupts a vehicle-target flight.
- [x] Merge town and uncategorised map flights into one option, retaining
      `gui.fly_towns`. Other destination switches remain independent. Release
      build and 48,600 assertions pass. Shared-map on/off, mid-flight cancellation
      and town-directory flight checked. Guide screenshot refreshed.
- [ ] Runtime news-item clicks and remaining secondary navigation controls.

User instructions: [prototype setup](smooth_zoom_readme.md).
Control details: [smooth zoom controls](smooth_zoom_controls.md).

Local evidence is in ignored `build-smoothzoom`: `validation-controls-full.txt`,
`runtime-controls-2.log`, `runtime-controls-3.log`, `runtime-snap.log`,
`runtime-limits.log`, `runtime-cancel.log`, `rapid-wheel-*.png`,
`controls-narrow.png` and `benchmark-palette-tracking.txt`. The Windows desktop
was locked during these checks. Game-window surface captures and in-game
screenshots were used. The automation helper's DPI coordinates were corrected.
Smoothness still needs subjective assessment on an unlocked, foreground window.

Latest evidence: `validation-fast-motion-full.txt`, `benchmark-fast-boundary.txt`,
`runtime-fast-final.log`, `runtime-fast-boundary.log`, `compact-fast-narrow.png`,
`compact-fast-coast.png` and `motion-final-*.png`. Before the motion sampler a
single transition had three updates in 305 ms (maximum gap 157 ms). The final
terrain capture run had 12 updates in 398 ms (maximum gap 36 ms). The capture-free
coastal four-notch run had 21 updates in 573 ms (maximum gap 35 ms). Moving-image
centre sampling is about 3-4 times faster than settled area filtering in the
synthetic benchmark. Captures confirm filtering returns after settling. They
cannot establish subjective smoothness. No game error dialogs were found.

## Perceptual flight update

- [x] Replace distance-based timing with van Wijk-Nuij perceptual path length.
- [x] Derive height from distance and configurable curvature. Flatten to zoom limits.
- [x] Avoid extra pull-back within one starting viewport width.
- [x] Bound animated duration to 300-2200 ms. Longer trips now accelerate
      to the cap instead of cutting (supersedes the initial >2500 ms jump).
- [x] Preserve fractional arrival zoom for ordinary destinations.
- [x] Respect smooth scrolling and cancel on any actual key press.
- [x] Blend vehicle location flights toward the live position in the final 20%.
- [x] Standalone numerical regression: 3,273 assertions in 13 cases, including the published d3 example.
- [x] Full Release rebuild and integrated tests: 13,627 assertions in 130 cases.
- [x] Runtime 305 ms short hop with native pan, approximately 1.47 s default
      town arc, 0.89 s fast arc, fixed 50% arrival and immediate long-trip cut.
      Smooth-scroll off prevented flights. Key/wheel takeover cancelled them.
      Keyboard-cancelled land remained pixel-identical beyond planned arrival.
- [x] Refresh four real town-flight recordings and capture pacing controls.
      No recorded land-crop changes after arrival in these paused fixtures.
- [x] Moving bus flight completed in 1.22 s and retained 130% arrival zoom.
- [ ] Runtime moving vehicle news references and all other vehicle approaches.
- [x] Initial 2048-square direct-cut check completed. Superseded by the
      animated long-trip policy. Cross-map model regression now checks an
      active 2200 ms flight and exact arrival.
- [ ] Subjective tuning on a 2048-square map.

## Retrofit and compatibility

- [x] Document existing-save retrofit, local-only options, integer camera save/load,
      rollback version limits, renderer requirements and mod-type expectations.
- [ ] Stock-created save -> patched save -> matching stock build round trip.
- [ ] Representative 8-bit and 32-bit / extra-zoom NewGRF runtime coverage,
      including custom infrastructure, animated sprites, articulated vehicles and trams.
- [ ] Multiplayer with matching patched revisions and per-client camera settings.
- [ ] Classify and validate multiplayer Game Script camera redirects. Currently
      source now routes these through the script category. Runtime checks remain open.
- [ ] Validate any chosen engine patch pack after source integration.

## Repeated navigation and ready-to-use demo

- [x] Reproduce the reported town sequence: a long third route hit the automatic
      >2500 ms cut after earlier animated trips. Keep all enabled trips animated,
      accelerating longer routes to the 2200 ms cap. Regression covers that sequence.
- [x] Release build and 13,649 assertions in 131 cases pass.
- [x] Check an industry trip visibly zooms out (66% captured during the arc).
      Nearby same-zoom trips remain flat under the earlier no-bob rule.
- [x] Check in a paused populated navigation save with base stations and nine
      vehicles, plus a launcher staging matching assets and AI scripts. Enable
      the main flight switch in its isolated demo config. All category defaults apply.
- [ ] Optional small arcs for nearby industry hops. Keep the current no-bob rule unless requested.

## Current follow-up

- [x] Validate 130% dragging: captured median update interval 109 -> 32 ms. No static-crop changes beyond 300 ms after release. Native pixel-scroll code remains unchanged.
- [x] Paused follow camera retains stable land across captures 700 ms apart.
- [x] Generated defaults regression and runtime bus centre-button flight/vehicle opt-out checks.
- [x] Consecutive town trips retain 130% arrival zoom and all remain animated.
- [x] Full final Release build: 13,700 assertions in 132 cases pass.
- [x] Refresh Basic master / Expert override screenshots and consecutive-town animation.
- [x] Remove the post-pan / stopped-follow area-filter redraw: retain the same
      fractional sampler at rest, without adding a cache. Document the fine-detail tradeoff.
- [x] Inspect the immediate release interval, including times before 300 ms:
      last static-land change at 87 ms while final updates completed, then stable.
- [x] Native snap and subsequent wheel settling remain valid. All 13,700 assertions pass.
- [ ] Broader pan-performance checks at larger resolutions and with complex NewGRFs.

## Long-term visual additions

- [ ] Standalone optional clouds effect layer mod to make large fly-outs feel more realistic. Investigate depth, transparency and a bounded rendering budget.
- [x] Smooth scaling for signs and labels, including readable text, positioning and click targets throughout fractional zoom (see implementation notes below).

## Destination arrival fitting: 10 October 2026

- [x] Choose arrival zoom from destination size rather than departure zoom. Fit within 80% of the viewport, use native levels and respect zoom limits.
- [x] Fit station spreads, industry footprints, estimated town extents and complete vehicle consists, including offscreen parts.
- [x] Preserve explicit town fixed/Keep current zoom overrides and per-object flight switches.
- [x] Release build and all 13,722 assertions in 134 cases pass.
- [x] GUI checks: airport 100%, small road station/bus/coal mine 200%. Refresh town and vehicle animations. This supersedes the earlier default zoom-retention behaviour.
- [ ] Validate long, reversing and overlapping NewGRF vehicles, irregular industry footprints, dispersed towns and unusually tall sprites across larger viewports.

## Fractional landing jitter follow-up

- [x] Remove the late motion-to-area sampler handoff for every fractional viewport, including wheel and flight arrival. Native rendering is unchanged. Fine detail stays centre-sampled with the selected sprite mipmap. This supersedes earlier notes about restoring area filtering.
- [x] End the flight subpixel tail when combined remaining pan and scale error is at most one pixel.
- [x] Regression coverage for 130%/84% redraw partitioning and bounded fractional flight landing at multiple viewport sizes. Release: 16,024 assertions / 135 cases pass.
- [x] Record fractional wheel and 130% flight landings: static land remains unchanged for 2.4 and 3 seconds respectively after its final movement. Fractional drag also shows no changes beyond 300 ms after release.
- [ ] Extend settling checks to complex NewGRFs, higher resolutions and unpaused scenes, distinguishing camera changes from animated world content.

## Continuous map labels

- [x] Apply one readability-bounded scaling curve to towns, stations, waypoints and signs at both fractional and native zoom levels.
- [x] Scale text, backgrounds, frames, click bounds, culling margins and dirty rectangles together.
- [x] Preserve interface font sizing and legacy-blitter behaviour. Keep normal map-label content through zoom transitions.
- [x] Reuse per-label scratch rasters. Handle offscreen pitch and palette-animation state safely, with no persistent text cache.
- [x] Test scaling continuity, monotonicity, readability limits and transparent/coloured coverage composition.
- [x] GUI checks at 12.5%, 84%, 130%, 200%, 400%. Station edge click, custom sign editor and town window pass. Settled labels stay unchanged across 700 ms captures.
- [x] Add station/sign and town scaling animations to the user guide.
- [ ] Broader multilingual/RTL, unusual fonts, high interface scales, dense NewGRF station names and waypoint-specific visual checks.
# Upstream requirements audit: 10 October 2026

- [x] Research contribution policy, coding style and CI requirements. Record [submission limits and technical checks](validation/upstream-readiness.md).
- [x] Register six new core headers in the CMake source inventory.
- [x] Add a repeatable full-build/source-description/CTest runner. All 144 local tests pass, including four upstream regressions.
- [x] Enable source-description and unused-string checks on private master pushes.
- [ ] Verify remote platform matrix and Linux string-check results. Compare Doxygen warnings with upstream base.

