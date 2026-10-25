# Graphics and renderer coverage

This records representative runtime checks, rather than a guarantee for every
NewGRF, font, driver or graphics card. Results dated 10 October 2026.

## OpenGFX 8.0

All eleven configurations passed with the paused `zoom-navigation.sav` fixture:

| Configuration | Checks |
| --- | --- |
| 32bpp-anim, simple, optimized, SSE2 and SSE4 | 100%, 130%, 200%, 84%, 25%. Stable fractional rest. Save/load |
| 40bpp-anim with Windows OpenGL | Same zoom, settling and save/load checks |
| 8bpp-optimized | Native rendering and unsupported fractional request remain safe |
| 1920 × 1080 | Same zoom, settling and save/load checks |
| 200% interface scale at 1600 × 1000 | Same zoom, settling and save/load checks |
| Arabic and Japanese interfaces at 1280 × 800 | Same zoom, settling and save/load checks |

Software and OpenGL native-200% label crops matched pixel for pixel. The
OpenGL check exposed a background-composition issue: palette indices can live
separately from RGB pixels. Scaled labels now resolve those colours through the
blitter before blending their text edges.

The release unit suite passes 68,807 assertions in 140 cases after this fix,
including GPU-format decoding, palette/partial-alpha sprites and clipped-source
checks. Native SSE palette and alpha arithmetic can differ from the scalar
compositor by one RGB unit. Opaque non-palette comparisons remain byte-exact.
The runtime checks are functional checks, not comparative frame-rate benchmarks.

## Additional graphics sets

OpenGFX2 Classic 0.8 and OpenGFX2 High Def 0.8 each passed all eleven
configurations in the table above. Classic supplies palette artwork. High Def
supplies 32-bit extra-zoom artwork. Native and fractional rendering therefore
exercise different source formats with the same camera and label code.
Downloads come from the
[official OpenGFX2 0.8 release](https://github.com/OpenTTD/OpenGFX2/releases/tag/0.8).
Downloaded graphics remain local and are not bundled with this modification.

The [machine-readable results](zoom-matrix-results.json) record the 33 passing
configurations, individual checks, local artifact paths and archive SHA-256
hashes. Early High Def attempts sent input before sprite loading completed.
The harness now waits longer at startup and after loading a save. Two interrupted
or incomplete runs were repeated successfully. Original Transport Tycoon
graphics and zBase have not been tested in this pass.

## Repeating the matrix

Stage the prototype assets with `launch-prototype.ps1`, install Pillow into
`build-smoothzoom/capture-deps`, and run:

```powershell
py -3 docs/validation/run-zoom-matrix.py --case all --graphics OpenGFX
py -3 docs/validation/run-zoom-matrix.py --case all --graphics "OpenGFX2 High Def"
```

Each run owns its game process and stores screenshots, console logs and JSON
results under `build-smoothzoom/coverage`. Graphics archives must already be in
`build-smoothzoom/baseset`. High-definition graphics need a longer startup wait.

## NewGRF and text fixture

`ZoomCoverage` extends the vehicle fixture with tram stops, waypoints, custom
company colours and Arabic, Japanese and accented Latin signs. Install its
folder as a local AI together with the official OpenGFX2 tram, tree, station and
object GRFs. Its Unicode source uses a UTF-8 BOM, required by the Squirrel loader.

Arabic and accented Latin signs were checked with Segoe UI. Japanese signs
were checked with Yu Gothic at 130%. A font without the
requested characters displays missing-glyph boxes. Scaling cannot supply absent
glyphs. In the unpaused fixture, tram 10 changed tile from 3257 to 3254 at speed
40. The truck and four trains also reported changing positions and nonzero
speeds in the AI debug window. Those historical checks exercised infrastructure,
company colours, wires and trees while zooming. Changing vehicle and animated
world pixels are expected. This is not a paused-scene settling comparison.
Interface-language checks above are separate from these custom-sign checks.

The extended fixture also builds eight objects, including a gasometer,
substations, a transmitter, a lighthouse and forests. The objects and their
shadows were checked at fractional zoom, including the game's transparency
toggle. The fixture's construction log identifies their locations for repeatable
camera checks.

![NewGRF objects and their labels during zoom](../images/smooth-zoom/newgrf-objects.webp)

## Pan performance

A capture-free nine-second right-drag check on the paused OpenGFX fixture,
using `32bpp-anim` and `win32:no_threads`, measured:

| Window | Native 100% | Fractional 130% before | Fractional 130% after |
| --- | --- | --- | --- |
| 960 × 600 | about 60 FPS | about 47 FPS | about 60 FPS |
| 1920 × 1080 | about 60 FPS | about 16 FPS | about 58 FPS |

These are this machine's measurements, not a hardware-independent promise.
Fractional pan now retains a pixel-aligned rendering origin at a fixed scale,
allowing it to copy pixels and redraw exposed strips. The displayed origin stays
within half a screen pixel of the requested position. Picking and dirty bounds
use that same origin, and animated palette dependencies move with copied pixels.
Changing the scale still redraws the viewport.

Both the original and final measurements ran without screenshot capture or
another active matrix. A palette-resolution experiment did not establish a useful
speed improvement and was removed. The final 512-sample viewport drawing means
were about 2.4 ms at 960 × 600 and 4.1 ms at 1920 × 1080 during the pan check.

All eleven OpenGFX configurations pass the updated matrix. Supported renderers
add 130%, 84% and 66⅔% pan checks, proving that the scene moves and that the
paused land/label crop matches a forced full redraw. The additional checks also
pass with High Def software/OpenGL and Classic at 1080p. These checks validate
the copied pixels rather than relying only on apparent steadiness.

![Fractional pan with retained sampling phase](../images/smooth-zoom/fractional-pan-optimised.webp)

In this paused object/coast fixture, the recorded land crop last changed at
2,222 ms, before the right-button release at 2,400 ms, and stayed unchanged
through the final capture at 6,523 ms. Palette animation and moving world content
remain separate from camera settling.

## Existing saves

A stock build of upstream revision `fab8089e6bed2be570b6b0a2cb9f19a5db16a7da`
created a new save. The modified build loaded it, selected 130% zoom and saved
it. The stock build then reopened that save successfully. Screenshots and console
logs are under `build-smoothzoom/stock-profile`. The test can be repeated with:

```powershell
py -3 docs/validation/run-save-roundtrip.py --stock <path-to-stock-openttd.exe>
```

The fractional view is client state. The save retains an integer zoom value.
This test used OpenGFX without NewGRFs. A save that uses NewGRFs still needs
those NewGRFs available when loaded, as in stock OpenTTD.

## Rendering optimisation checks, 11 October 2026

The fractional row changes passed all eleven OpenGFX configurations listed
above. All supported configurations also passed pan-versus-complete-redraw
checks at 130%, 84% and 66.7%. OpenGFX2 Classic and High Def each passed the
SSE4 rendering, stable-rest, pan redraw and save/load checks. These repeat the
functional checks after the row compositor and animated-colour cache changes.
They do not establish smooth foreground playback.

The complete local suite passed 149 CTest cases. The smooth-zoom subset passed
51 cases with 77,632 assertions, including palette refresh, repeated draws,
sprite re-encoding at the same address and bounded-cache eviction.

[All eight platform builds and tests](https://github.com/Deku-shrub/OpenTTD/actions/runs/38178692367)
passed on `04833375bd`. The
[Doxygen comparison](https://github.com/Deku-shrub/OpenTTD/actions/runs/38178691465)
added no warnings. Source-description and English-string checks passed.
The [published release](https://github.com/Deku-shrub/OpenTTD/releases/tag/build-38178692367-04833375bdf8)
contains Windows x64/x86, Linux x64 and macOS arm64 packages. All four manifests
identify `04833375bdf855b2a666e2a8706d114acc45eedc` with no local modifications.

Foreground smoothness, fitted-arrival appearance and replacement promotional
footage remain open. Windows was locked during the desktop-capture attempt.
See the [performance and capture notes](promotional-captures.md).

A follow-up cache-budget guard rejects large textures before storage-size
multiplication can wrap on a 32-bit build. Its oversized-source regression
needs no source pixels or large allocation. After this correction, all 150
local CTest cases passed. The smooth-zoom subset passed 52 cases and 77,633
assertions. Normal-sized sprite sampling is unchanged.

Renderer tests now check SSE4.1 availability before exercising the SSE4 row
compositor and also compare the SSE2 animation renderer against the scalar
reference. The cache-reuse test falls back to SSE2 on older CPUs. The expanded
smooth-zoom subset passed 79,745 assertions in 52 cases. Camera test loops also
use references to remove the two GCC range-loop copy warnings.

## Release checkpoint, 12 October 2026

[All eight platform jobs and package publication](https://github.com/Deku-shrub/OpenTTD/actions/runs/38181538021)
passed for `e761771dd8`. Windows x64 and x86 each passed 150 CTest cases.
The [final Doxygen comparison](https://github.com/Deku-shrub/OpenTTD/actions/runs/38181537992)
added no warnings, and source-description/string checks passed. Compiler logs
confirmed that the two camera-test range-loop warnings were removed. The
remaining compiler warnings are in unchanged road and industry code.

The [latest tested release](https://github.com/Deku-shrub/OpenTTD/releases/tag/build-38181538021-e761771dd8b8)
has four packages. Their manifests all identify clean revision
`e761771dd8b8b485dd142ef60128b55c81bb2512`.

[Intel macOS and MinGW checks](https://github.com/Deku-shrub/OpenTTD/actions/runs/38179809173)
passed on the rendering commit `04833375bd`. Its
[CodeQL scan](https://github.com/Deku-shrub/OpenTTD/actions/runs/38178692288)
passed and added no alerts relative to the preceding fork scan. Existing alerts
remain in the repository. The
[final revision's CodeQL scan](https://github.com/Deku-shrub/OpenTTD/actions/runs/38181538017)
passed with no new alert IDs relative to the preceding fork scan. Existing
alerts remain open.

The original fitted-city trace reached the intended Chindingwell destination.
It recorded only ten camera updates in 2141 ms, with a 429 ms maximum interval.
This confirms poor cadence in that take, without resolving its reported
vertical appearance. The desktop was subsequently unlocked and foreground
testing resumed. Replacement promotional captures remain unfinished.

On the unlocked desktop, the town flight produced 71 camera updates in 1887 ms
with a 63 ms maximum interval. The fitted-city flight produced 78 updates in
2118 ms with a 52 ms maximum interval. The city zoom-out produced 47 updates in
1297 ms with a 40 ms maximum interval. The rail measurement overlapped a
recorder experiment and is excluded from the uncaptured comparison.

Guarded desktop duplication recorded only the owned, foreground 1920 x 1080
game. Direct D3D11 BGRA input was rejected by the AMD encoder, so the successful
test downloaded frames and converted them to NV12. At 60 fps, the recorded
town flight produced 52 camera updates with an 86 ms maximum interval. At
30 fps it produced 56 updates, with a 73 ms maximum interval using AMD encoding
and 78 ms using software encoding. Recording overhead remains measurable.
These local recordings are QA output, rather than approved promotional takes.

## Dense-view follow-up, 12 October 2026

The rail scene was profiled without recording at 1920 x 1080 with the SSE4
animation blitter and Windows driver. CPU samples were resolved against the
exact executable's linker map. Pixel composition dominated the identified game
code, particularly short row tails. Opaque samples still read the destination
before the compositor's internal alpha shortcut.

The retained change copies opaque fractional pixels without reading the
background. Resolved palette colours retain the compositor's 255 output alpha.
SSE samples use individual values instead of a temporary array. In the Windows
release build, this removed per-row stack-buffer setup and cookie checks.
Translucent blending, transparent pixels and animation metadata keep their
previous behaviour. Cache size, native drawing and saved settings are unchanged.

The benchmark used eight wheel notches over one second, from 100% to 25%,
anchored at the viewport centre. The same original executable was tested before
and after the optimized build to check timing drift. No recorder or video
encoder ran during these measurements.

Both executables had smooth zoom enabled. These figures compare the previous
fractional renderer with the optimized renderer, rather than with the feature
disabled. Dense-map performance is assessed against feature-disabled rendering
at matching scene density. The base game's rendering cost is acceptable. Extra
cost from fractional rendering needs separate measurement and optimization.

| Traffic | Original mean interval (ms) | Optimized mean interval (ms) | Reduction | Optimized camera updates per second |
| --- | ---: | ---: | ---: | ---: |
| Paused, warm | 37.2 | 34.4 | 7.7% | 29.1 |
| Moving | 39.0 | 36.6 | 6.0% | 27.3 |

These are means of per-run averages. Paused figures exclude the first cold run
of each group. The comparison includes six warm paused and eight moving runs
of the original executable, and three warm paused and four moving runs of the
optimized build. The first paused run improved from a mean 42.5 ms to 38.8 ms.
An optimized moving run still had a 57 ms maximum interval. The gain does not
establish consistently smooth wide-view rendering.

The local build passed all 150 CTest cases. The smooth-zoom subset passed 86,273
assertions in 52 cases, including static white remapped to an animated colour.
Source-description checks passed. Normal release linking was restored after
profiling. Software, simple, SSE4 and OpenGL runtime checks also passed at 100%,
130%, 200%, 84% and 25%, including static rest, save/load and agreement between
pan and complete redraws at 130%, 84% and 66.7%. No additional promotional clips
were recorded during this work.

### Feature-disabled baseline

The performance requirement is to avoid additional dense-map slowdown from
the feature, rather than meet a fixed frame-rate target. The feature-disabled
baseline below uses this fork's native renderer. It is not a separate upstream
binary.

Two moving-traffic runs at the same rail location used an eight-second
continuous horizontal pan without recording. At 25%, viewport drawing averaged
0.71 ms with smooth zoom disabled and 0.73 ms with it enabled. Both maintained
about 60 drawing updates per second. At a fractional 33.33%, viewport drawing
averaged 1.28 ms and also maintained about 60 updates per second. The fractional
view has a different field of view, so its timing is not a matched-density
comparison.

Those pan runs used the later discarded SIMD experiment. Its changes affected
only fractional row writes. The native-level comparison used the unchanged
native path in both cases. The full-redraw comparison below used the retained
renderer.

A separate test repeatedly requested complete screen redraws for eight seconds
using an isolated refresh-screen hotkey. It used the retained renderer from
`5e98f414a61b`, moving traffic and the same 1920 x 1080 rail view. A scale of
25.001% keeps the field of view within 0.004% of the native 25% view while
exercising fractional rendering. The console confirmed the rendering path.
Each row averages two runs of the console's most recent 64 viewport drawing
measurements. The refresh test measures sustained redraw cost, not a flight or
zoom animation's frame cadence.

| Scale and setting | Viewport drawing (ms) |
| --- | ---: |
| Native 25%, smooth zoom disabled | 19.70 |
| Native 25%, smooth zoom enabled | 18.83 |
| Fractional 25.001%, smooth zoom enabled | 33.37 |

There is no measured native-level regression in these runs. The fractional
redraw still costs about 13.7 ms more than the disabled baseline, roughly 69%.
The previously measured 6-8% optimization does not establish native parity.
Further fractional redraw optimization remains pending. No additional clips
were recorded.

A later experiment replaced scalar alpha checks with vector checks and loaded
contiguous groups of four samples together. Repeated zoom-out measurements did
not improve, so that experiment was removed.

All eight platform build jobs and Doxygen checks passed for `5e98f414a61b`.
All four published platform manifests identify that revision with an unmodified
source tree. Its CodeQL scan passed with no new alerts relative to the preceding
fork scan.


## Uncovered performance checks, 13 October 2026

The whole-sprite sampling shortcut passed 150 unit cases and 86,597 smooth-zoom
assertions. Boundary tests compare its pixel selection with the previous
sampling rules, including negative origins and clipped axes.

Matched 1920x1080 background CPU measurements used the downloaded OpenTTD 13.4
network, moving traffic and SSE4 animation. Both builds had the same diagnostics.
F6 sampled timings with the console closed. Three eight-second repetitions gave
these median viewport drawing durations for forced full redraws:

| Scale and setting | Previous renderer | Candidate |
| --- | --- | --- |
| 25%, smooth zoom Off | 27.13 ms | 27.80 ms |
| 25%, smooth zoom On | 27.19 ms | 26.95 ms |
| 25.001%, smooth zoom On | 44.64 ms | 42.65 ms |
| 33.333%, smooth zoom On | 38.84 ms | 37.35 ms |

Fractional redraw cost fell about 4-5%. Native results varied between runs.
The near-native fractional case still cost about 53% more than the candidate's
fixed-zoom control. Performance parity remains unresolved. These uncovered
measurements supersede earlier console-obscured comparisons for this protocol.
Background CPU measurements do not establish foreground presentation smoothness.

A 32-cycle moving-traffic run measured fractional pan at 0.96 ms before and
0.95 ms after repeated zooming. Private bytes rose from 279.85 MiB to 294.24 MiB
and stayed at that value for the last six cycles. A paused counterpart rose
from 269.62 MiB to 271.08 MiB, with small increases still present near its end.
Neither run proves that longer sessions have no memory growth. The paused
viewport's recent CPU samples were zero at rest and are not a useful pan-cost
comparison.

Vehicle-following checks passed at 100%, 130% and 33.333%, with vehicle 1
reported as the followed vehicle. Separate single-run diagnostics covered
hidden labels, disabled palette animation, 1280x720 and OpenGL. Hidden labels
reduced fractional redraw cost. OpenGL's fractional redraw samples were much
higher than its native samples, reaching 93.15 ms near 25%. Repeat and profile
that path before treating OpenGL as performance-ready. Single-run differences
are diagnostic leads, not measured optimization gains.

Raw logs and JSON remain in ignored local output under
`build-smoothzoom/performance`. No promotional recordings were made for these
runs.

The unlocked foreground interaction run completed all nine checks with the
owned game in front. Slow zoom averaged 38.9 ms between camera updates, with
a 60 ms maximum. Fast zoom averaged 43.4 ms, with a 75 ms maximum. Reversal
averaged 32.7 ms, with a 50 ms maximum. The short flight arrived in 347 ms with
18 frames and a 23 ms maximum interval. The long flight arrived in 2087 ms
with 74 frames and a 58 ms maximum interval. Pan and wheel input both cancelled
the long flight. Disabled navigation produced no flight traces. Passing these
behaviour checks does not mean dense zoom or flight cadence is satisfactory.
Two foreground dense-view repetitions completed all 16 cases. Median full
redraw durations were 27.36 ms at 25% with smooth zoom Off, 28.43 ms at 25%
with it On, 44.16 ms at 25.001%, and 37.31 ms at 33.333%. Continuous-pan samples
were 0.56 ms, 0.53 ms, 1.58 ms and 0.84 ms respectively. All cases retained
foreground focus. These checks confirm that the fractional redraw overhead
also appears in foreground runs. The small native timing difference needs
longer matched repetitions before attributing it to the feature.

## Maintenance refactor checks, 25 October 2026

The extracted camera module and shared vehicle navigation helper passed the
Release build and all 150 CTest tests. The full fork diff passed the official
OpenTTD diff-style checker and the source-file description check.

An isolated Windows game on the downloaded OpenTTD 13.4 network passed nine
zoom, flight, opt-out and takeover checks. Five further checks confirmed one
vehicle-category flight per enabled centre click, no flight with the vehicle
category disabled, and vehicle following at 100%, 130% and 33.33%. These were
background behaviour checks, not foreground smoothness measurements.

The fixture staging script produced all four NoAI directories. Each dependent
AI's generated `base.nut` matched `ZoomStatic/main.nut` byte for byte. Local logs
remain under ignored `build-smoothzoom/qa-refactor-20261025` and
`build-smoothzoom/qa-refactor-vehicles-20261025`.
