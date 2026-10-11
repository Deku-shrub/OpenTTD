# Promotional capture notes

Recorded 11 October 2026 using the published fork build `5bb7002e81fb`.
The raw footage is in ignored local output at `build-smoothzoom/promo/takes`.
The editing index is `build-smoothzoom/promo/README.md`. It includes links to
the clips, their settings, a contact sheet and capture limitations. Raw videos
and individual vehicle stills are not committed documentation media.

The map is 2TallTyler's OpenTTD 13 title-game network, downloaded from
[upstream tag 13.4](https://github.com/OpenTTD/OpenTTD/blob/13.4/media/baseset/opntitle.dat).
The [competition page](https://github.com/OpenTTD/OpenTTD/discussions/10210)
provides author credit. The upstream title-file commit is
`4b123394cf5102688d9635d2988a13dbc9018d4d`.
The map has 27 towns, 71 trains, 321 road vehicles, 14 aircraft and 40 ships.
No network was built for filming. A paused copy provides repeatable starts.
Unused title-game AI/Game Script references are unavailable. Existing traffic
runs without them. No NewGRFs are required.

The set contains six classic/enhanced pairs: city zoom, rail and river zoom,
town navigation, vehicle-location navigation, destination fitting and manual
takeover. Four extras cover a smaller flight zoom-out allowance, the airport,
the river network and oil shipping. All 16 MP4s are 1920 x 1080, 16:9, with a
60 fps output timeline and no audio. Each has an unannotated 10-12 second take,
including time before the action and after arrival.

Classic takes disable smooth wheel zoom and flights in the same executable.
Ordinary smooth scrolling stays enabled. Shared renderer and label changes
remain present, so these compare controls rather than stock binaries. Zoom
pairs use matching magnification endpoints, with two classic wheel steps or
eight fractional steps per two-octave change. Most flights use default pacing.
The smaller fly-out explicitly changes the extra zoom-out allowance to one.

Camera traces confirm a 200% vehicle arrival and a 50% city arrival from a
200% departure. The takeover recording interrupts the flight with a drag.
These are representative scenes, not exhaustive graphics or performance checks.

The recordings exposed low camera cadence during wide flights on this dense
map at 1080p. A default town flight produced 10 camera updates while recording.
A repeat without recording produced 22 in 1.89 seconds, with a 145 ms maximum
interval. Recording adds overhead, and the uncaptured flight still falls below
the desired cadence. The one-extra-level take did not improve recorded cadence.
Repeat without recording before drawing performance conclusions about a change.

Output frames repeat when capture misses a deadline. Playback timing is retained.
No motion interpolation or speed changes were applied. A 60 fps MP4 does not
establish 60 distinct game frames. Black corners in some views are map edges.
Final clips exclude the console and desktop borders. The game toolbar remains
visible. Capture metadata and checksums stay beside the footage.

Select broad scenes for any later documentation excerpts. Compress those
separately and retain only current behaviour in the end-user gallery.

## Smoothness QA

User review found poor smoothness in all ten enhanced scenarios. Manual takeover
worked, but the city arrival also appeared to rise vertically rather than follow
the expected route. These takes remain debugging evidence and are not approved
promotional footage.

Uncaptured repeats on the same network at 1920 x 1080 reproduced the stalls.
The city zoom-out produced 14 camera updates over 1324 ms, with a 130 ms maximum
interval. The rail zoom-out produced 13 over 1368 ms, with a 161 ms maximum.
Town and fitted-city flights each produced 22 updates, with maximum intervals
of 169 ms and 151 ms. Rendering and camera-path investigation remains open.

Rendering optimisation checks on 11 October used the same paused network and
1920 x 1080 viewport, without video recording. Row batching, native SSE colours,
transparent margins and four-pixel opaque/binary-alpha composition reduced the
city zoom-out to 41 updates in 1301 ms (53 ms maximum), the rail zoom-out to 28
in 1330 ms (74 ms maximum), the town flight to 63 in 1889 ms (68 ms maximum),
and the fitted city arrival to 61 in 2145 ms (81 ms maximum).

A bounded cache of resolved animated SSE colours was then tested twice. City
zoom-outs produced 48 and 42 updates, rail zoom-outs 27 and 30, town flights
74 and 64, and fitted arrivals 55 and 75. Maximum fitted-arrival intervals
varied from 144 to 56 ms. The cache uses a 4 MiB accounting limit, a 128 KiB
entry limit and at most 256 entries. Large sprites retain the scanline path.
Regression tests cover palette changes, repeated draws, re-encoding at the same
address and eviction. These measurements show progress, but wide rail views
still averaged 45-47 ms between camera updates. The smoothness issue remains open.

Camera-centre measurements followed the direct town-to-town segment within
0.55 screen pixels. This does not close the reported vertical appearance of
the city arrival. It still needs review in foreground footage.

GDI recording with software or AMD AMF encoding reduced camera cadence in the
previous tests. A GPU desktop-duplication capture was attempted, but stopped
before recording because Windows was showing its lock screen. Foreground
smoothness and replacement promotional captures remain unverified.

Local validation of the rendering changes passed all 149 CTest cases, including
51 smooth-zoom cases and 77,632 assertions. The source-description checker
passed. OpenGFX software, SSE4 and OpenGL runtime checks passed fractional
rendering, unchanged static land after rest, save/load and agreement between
pan redraws and complete redraws at 130%, 84% and 66.7%. The 8bpp renderer
retained native rendering and safely rejected unsupported fractional requests.

The remaining seven local configurations also passed: simple, optimized, SSE2,
1920 x 1080 software, 200% interface scale, Arabic and Japanese. OpenGFX2 Classic
and High Def each passed the SSE4 checks. The published `04833375bd` revision
passed all eight platform jobs and the Doxygen comparison. See the
[coverage checkpoint](zoom-coverage.md#rendering-optimisation-checks-11-october-2026)
for the CI and download links.
