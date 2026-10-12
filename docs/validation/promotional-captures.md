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
before recording because Windows was showing its lock screen. On 12 October,
the desktop was unlocked and guarded desktop duplication succeeded at full HD.
The foreground town flight produced 71 camera updates in 1887 ms without
recording. Desktop duplication with AMD encoding produced 52 updates at 60 fps
and 56 at 30 fps. Software encoding at 30 fps also produced 56 updates. Frame
download and colour conversion are still required with the installed FFmpeg
build. These tests remain local QA clips. They do not replace the failed
promotional takes. See the [foreground results](zoom-coverage.md#release-checkpoint-12-october-2026).

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

## Foreground scenario checks, 12 October 2026

The clean `e761771dd8` Windows build was retested at 1920 x 1080 on the unlocked
desktop without recording. Each scenario reset its windows before navigation.
The input runner uses physical coordinates and the current vehicle controls.
A preliminary run selected the wrong town and was discarded. The corrected
run asserted both town destinations, the train target, 100%, 50% and 200%
arrival scales, and interruption by manual dragging.

| Scenario | Camera updates | Active duration (ms) | Average interval (ms) | Maximum interval (ms) |
| --- | ---: | ---: | ---: | ---: |
| City zoom-out | 45 | 1288 | 28.6 | 59 |
| Rail and river zoom-out | 29 | 1313 | 45.3 | 77 |
| Town flight | 74 | 1894 | 25.6 | 64 |
| Train arrival | 68 | 1961 | 28.8 | 52 |
| Fitted city arrival | 79 | 2117 | 26.8 | 49 |
| One-extra-level flight | 72 | 1804 | 25.1 | 53 |
| Airport zoom-out | 36 | 1267 | 35.2 | 42 |
| River zoom-out | 36 | 1313 | 36.5 | 49 |
| Oil shipping zoom-out | 52 | 1271 | 24.4 | 30 |

The one-octave zoom figures combine the active intervals of four wheel steps.
The manual takeover trace started a flight but did not report an arrival after
the drag. Moving traffic was enabled in the zoom and train scenarios.

Rendering is faster than in the original takes, but dense rail views still
fall below 30 camera updates per second. Occasional longer intervals also
remain. These measurements do not establish that all reported smoothness
issues are fixed. Replacement recordings need their own visual review.

Ten fresh enhanced recordings are in ignored local output at
`build-smoothzoom/promo/qa-foreground-takes`. Its `README.md` links every movie
and lists camera traces. Each 10-12 second clip uses full-HD desktop duplication
at 30 fps, AMD encoding and no audio. Variable frame timestamps retain capture
timing. No motion interpolation or playback speed changes were applied.
Capture stopped if the owned game lost the foreground. The test game was closed
after recording.

The new recorded town flight produced 56 updates in 1900 ms with a 71 ms maximum
interval. Fitted-city arrival produced 57 in 2143 ms with a 62 ms maximum, and
train arrival produced 51 in 1986 ms with a 75 ms maximum. Destination, arrival
scale and takeover assertions passed for these clips. Dense rail zoom-out still
averaged 54.1 ms between updates while recording. The clips remain QA output
pending visual review. The original footage has not been overwritten, and no
new clips have been embedded in end-user documentation.

Further capture work was deferred after the input timing review. The one-octave
wheel demonstrations spaced four events 333 ms apart, allowing individual
transitions to settle. They should be replaced with overlapping input for a
continuous gesture. Matching feature-disabled takes are also still required for
the new foreground set. Performance work now takes priority. See the
[dense-view follow-up](zoom-coverage.md#dense-view-follow-up-12-october-2026) for
the uncaptured comparison and remaining frame-rate limit.
