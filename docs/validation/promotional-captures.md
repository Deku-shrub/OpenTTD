# Promotional capture notes

Recorded 11 October 2026 using the published fork build `5bb7002e81fb`.
The raw footage is in ignored local output at `build-smoothzoom/promo/takes`.
The editing index is `build-smoothzoom/promo/README.md`; it includes links to
the clips, their settings, a contact sheet and capture limitations. Raw videos
and individual vehicle stills are not committed documentation media.

The map is 2TallTyler's OpenTTD 13 title-game network, downloaded from
[upstream tag 13.4](https://github.com/OpenTTD/OpenTTD/blob/13.4/media/baseset/opntitle.dat).
The [competition page](https://github.com/OpenTTD/OpenTTD/discussions/10210)
provides author credit. The upstream title-file commit is
`4b123394cf5102688d9635d2988a13dbc9018d4d`.
The map has 27 towns, 71 trains, 321 road vehicles, 14 aircraft and 40 ships.
No network was built for filming. A paused copy provides repeatable starts.
Unused title-game AI/Game Script references are unavailable; existing traffic
runs without them. No NewGRFs are required.

The set contains six classic/enhanced pairs: city zoom, rail and river zoom,
town navigation, vehicle-location navigation, destination fitting and manual
takeover. Four extras cover a smaller flight zoom-out allowance, the airport,
the river network and oil shipping. All 16 MP4s are 1920 x 1080, 16:9, with a
60 fps output timeline and no audio. Each has an unannotated 10-12 second take,
including time before the action and after arrival.

Classic takes disable smooth wheel zoom and flights in the same executable;
ordinary smooth scrolling stays enabled. Shared renderer and label changes
remain present, so these compare controls rather than stock binaries. Zoom
pairs use matching magnification endpoints, with two classic wheel steps or
eight fractional steps per two-octave change. Most flights use default pacing;
the smaller fly-out explicitly changes the extra zoom-out allowance to one.

Camera traces confirm a 200% vehicle arrival and a 50% city arrival from a
200% departure. The takeover recording interrupts the flight with a drag.
These are representative scenes, not exhaustive graphics or performance checks.

The recordings exposed low camera cadence during wide flights on this dense
map at 1080p. A default town flight produced 10 camera updates while recording;
a repeat without recording produced 22 in 1.89 seconds, with a 145 ms maximum
interval. Recording adds overhead, and the uncaptured flight still falls below
the desired cadence. The one-extra-level take did not improve recorded cadence.
Repeat without recording before drawing performance conclusions about a change.

Output frames repeat when capture misses a deadline. Playback timing is retained;
no motion interpolation or speed changes were applied. A 60 fps MP4 does not
establish 60 distinct game frames. Black corners in some views are map edges.
Final clips exclude the console and desktop borders; the game toolbar remains
visible. Capture metadata and checksums stay beside the footage.

Select broad scenes for any later documentation excerpts. Compress those
separately and retain only current behaviour in the end-user gallery.
