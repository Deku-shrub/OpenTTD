# Smooth zoom and fly navigation: user guide

[Download and install](install_smooth_zoom.md) · [Online guide and animations](https://github.com/Deku-shrub/OpenTTD/blob/master/docs/smooth_zoom_readme.md)

The small downloadable game packages include these written guides. Animated examples and test-map tools are available in the online guide and repository.

[Fork homepage](../README.md) · [Progress and TODO](smooth_zoom_todo.md) ·
[Test coverage](validation/zoom-coverage.md)

See the [implementation and validation checklist](smooth_zoom_todo.md) for
what is complete and what still needs work.

This experimental fork adds eased fractional mouse-wheel zoom, a compact toolbar
percentage/snap control, scaling labels and simultaneous pan-and-zoom flights.
Smooth zoom and flights are enabled by default in this fork and require a supported 32bpp
blitter. Fractional rendering can be slower than ordinary integer zoom.

## Get started

Clone or download [this fork](https://github.com/Deku-shrub/OpenTTD), then follow
[COMPILING.md](../COMPILING.md) for your platform's dependencies and build steps.
Run the resulting game with the matching language files and an installed base
graphics set. Official OpenTTD releases do not contain this fork's features.

Find **Smooth mouse-wheel zoom** in **Game Options > Advanced**; select the
Expert/all-settings filter and search for **smooth**. For animated navigation,
search for **fly** and check **Fly-to navigation**. See the
[zoom controls](#enable-smooth-mouse-wheel-zoom) and [flight settings](#fly-to-a-location)
below for the full controls and per-destination overrides.

Both master switches and all destination categories default to **On** in this
fork. Existing configuration files retain saved values, including **Off**;
the defaults apply when those entries are absent. You can disable either master
switch or individual flight categories normally. An eventual upstream proposal
would be expected to use disabled master defaults, if such a contribution
becomes eligible and is accepted; this fork deliberately showcases the features.

For a populated Windows demonstration, run the checked-in launcher from the
repository root, supplying your own built executable and installed OpenGFX 8.0
archive. For example, with a Visual Studio Release build in `build`:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\docs\validation\launch-prototype.ps1 -Executable .\build\Release\openttd.exe -GraphicsArchive "C:\path\to\opengfx-8.0.tar"
```

The launcher enables camera features in an isolated configuration and loads the
included test map. It requires a compiled executable and graphics archive; it
does not download them. Keep the terminal open while playing. See the fixture
instructions below for company selection and moving vehicles.

## Launch the prepared Windows prototype on the development machine

On this development machine, the executable is `C:\ottd-build\Release\openttd.exe`.
The prepared launcher stages the matching language and base-set files and uses
an isolated test configuration. Run this in PowerShell:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File C:\Users\chris\github\OpenTTD\docs\validation\launch-prototype.ps1
```

Keep the terminal open while playing. The checked-in launcher opens
`docs/validation/zoom-navigation.sav`, a populated, paused test world with base
stations, a bus, truck, four train types, ship, aircraft and helicopter, plus
infrastructure and industries. All flight categories
and smooth wheel zoom are enabled in its isolated demo configuration. Ordinary
installation defaults enable the features too. Unpause to move the vehicles.

Use the toolbar's station and vehicle company menus to select the fixture
company: the initial human company has no fleet. Vehicle lists contain names
such as **Zoom bus** and **Zoom truck**. Click a vehicle to open its window, then
use its location control; double-click that control to follow. Stations can
also be opened by clicking them directly on the map.

The launcher stages the matching language, base metadata and NoAI fixture files.
It accepts `-Executable` for another Windows build, `-GraphicsArchive` for an
installed `opengfx-8.0.tar`, `-Savegame` for your own world, or `-NewMap` for an
empty generated map. Audio is disabled. It requires a compiled game and OpenGFX;
this is not a standalone installation package. The older local `desktop.ps1`
helper also defaults to the populated map.

## Existing saves, rollback and mod compatibility

This is an OpenTTD engine patch, not a NewGRF or a downloadable content pack.
Use the patched executable and its matching language files; enabling a NewGRF
in an ordinary OpenTTD installation cannot add these camera controls.

### Retrofit an existing game

An existing save or scenario can use smooth zoom and fly navigation immediately
if the underlying OpenTTD version can already load that file. No new game,
map conversion, rebuilding of infrastructure or NewGRF-list change is required.
Load the game normally and enable the local options in Advanced settings. With
the prepared development launcher, add `-Savegame "C:\path\to\your-game.sav"`
to the launch command above instead of generating a test map.

Keep the save's existing NewGRF files, versions, order and parameters. This
patch does not relax ordinary content dependencies or NewGRF incompatibilities.
Changing a running game's NewGRF list is a separate operation and is unnecessary
for this feature; see the [official NewGRF guide](https://wiki.openttd.org/en/Manual/NewGRF).

Camera options are local configuration settings (`NotInSave`, `NoNetworkSync`),
not game-world settings. A save sent to another player does not enable the
feature for them. Flight progress and fractional zoom are transient: saving
stores the current camera position with a traditional zoom level and adjusts
the origin to preserve the viewed centre. Loading restores that integer level
and cancels any pending flight. The exact fractional percentage, destination
and remaining flight time are not preserved.

The patch adds no savegame fields, chunks or savegame-version bump. A game saved
here should therefore remain readable by an **unmodified build that understands
the same upstream savegame version**, provided its content dependencies are
available. A stock -> modified -> stock save/load round trip passed against the
matching upstream revision; see the [coverage notes](validation/zoom-coverage.md).
An older OpenTTD release, a newer-format save, or a save
from a patch pack with its own format extensions may still be incompatible.
To retain the option of returning to an older installation, keep the original
save and save prototype sessions under a different filename. Turning the camera
options off requires no game-data cleanup or uninstall operation inside the save.

### Content and renderer compatibility

The expectations below come from the code boundaries; only the stated coverage
has been exercised. No specific third-party NewGRF set is certified yet.

| Content or environment | Retrofit implications and current limits |
| --- | --- |
| Base graphics | OpenGFX 8.0, OpenGFX2 Classic 0.8 and OpenGFX2 High Def 0.8 passed the eleven-configuration runtime matrix, including fractional rest and save/load. Original TTD graphics and zBase are untested. |
| 8-bit NewGRF artwork | Expected to work through a supported 32bpp blitter, including palette remapping. **8-bit artwork is not the same as an 8bpp renderer**; switching the blitter does not require replacing those NewGRFs. |
| 32-bit / extra-zoom NewGRFs | Expected to load normally, without format or asset conversion. Different artwork at native sprite zoom levels can visibly change during a transition. Large sprites and high-resolution packs can increase redraw cost. These sets still need visual tests. |
| Vehicle, industry/cargo, house, object, station, track, road/tram and bridge NewGRFs | No deliberate content restriction or changes to their simulation rules. Custom sprite bounds, transparency, company colours, animation, articulated vehicles and trams remain important coverage gaps. Base infrastructure and representative base vehicles have been exercised; that does not validate every custom set. |
| Palette animation / translucency | Implemented in the supported rendering paths with remapping, alpha and dirty-region tests. Complex animated NewGRF scenes still need runtime validation. |
| AIs and Game Scripts | No AI/API format change is required. Script-triggered camera movement can animate when its flight option is enabled, changing presentation timing. Disable flights to preserve ordinary navigation behaviour. Local script navigation has a separate switch; multiplayer script redirects use the script switch; their runtime validation remains open. |
| Scenarios and heightmaps | Ordinary load/version/content requirements apply. A loaded scenario can use the camera options; an imported heightmap creates a world through the ordinary generator. |
| Sound/music packs | No audio format or playback changes are introduced by this patch. |
| Supported software 32bpp blitters | Runtime checks passed for simple, optimized, SSE2, SSE4 and animation variants on Windows with all three tested base sets. |
| 8bpp or null/headless blitters | Smooth zoom and animated flights are unavailable; normal navigation remains. A dedicated server does not need to render the effect. |
| OpenGL / `40bpp-anim` | Runtime zoom, fractional settling and save/load checks passed with all three tested base sets. GPU palette-backed label composition is corrected. Other GPU/driver combinations still need testing. |
| Multiplayer | Camera preferences are per client, but normal executable-revision and NewGRF compatibility checks still apply. This private build has a different revision from stock OpenTTD and ordinarily cannot join a stock server. Use matching builds and content for server/clients; multiplayer testing is still open. See the [official multiplayer guide](https://wiki.openttd.org/en/Manual/Multiplayer). |
| Other engine patches / patch packs | Not a plug-in for another executable. Port the source changes and rebuild. Changes to viewports, blitters, zoom, toolbar/window input or settings may conflict; patches with custom save formats add separate compatibility constraints. No patch-pack integration has been validated. |

### Current evidence and remaining checks

Existing saved validation worlds have been loaded repeatedly, including loading
during a flight. Infrastructure, representative moving vehicles and camera
cancellation have been checked. Save-format declarations and the savegame
version remain unchanged; the only save/load adaptation converts the camera
view to the existing integer representation. The latest full Release suite
passes 68,744 assertions across 142 unit cases, plus all four upstream
regression suites (146 local CTest tests). The graphics-set matrix and stock
save round trip are recorded in the [coverage notes](validation/zoom-coverage.md).
Source references:
[camera save/load](../src/saveload/misc_sl.cpp),
[local settings](../src/table/settings/gui_settings.ini), and
[network revision checks](../src/network/core/network_game_info.cpp).

Still required: broader combinations of NewGRFs, multiplayer with matching
builds, multiplayer script redirects, and integration
with any intended patch pack. Keep these separate from the validated base-game
coverage in the [TODO list](smooth_zoom_todo.md).

## Enable smooth mouse-wheel zoom

Open **Game Options > Advanced** (the settings page), rather than the Graphics tab.
Select **Expert / all settings** in the settings filter, search for **smooth**,
and enable **Smooth mouse-wheel zoom**.

Alternatively, open the console using the key below Escape and enter:

```text
set gui.smooth_zoom 1
```

Close the console and scroll the mouse wheel over the map. Four wheel steps
double or halve the magnification. The updated controller eases into the target
and keeps its motion when further notches arrive. Zoom buttons continue to
select integer levels.

Fractional views now use the same mipmap-based pixel-centre sampler during
wheel zooming, flying, panning, following and rest. This removes the late
switch to area filtering that could change fine detail around apparent arrival.
Fine textures remain slightly sharper at rest rather than being averaged again.
The nearly invisible wheel easing tail ends once residual scale displacement is
at most one pixel across the viewport. Flights also finish once their combined
remaining pan and scale displacement is below one pixel. Native zoom levels
continue to use the original integer renderer.

The updated main toolbar displays a compact percentage button, such as **84%**.
Its tooltip shows the nearest traditional level. Click it to stop the transition
and snap to that traditional level. The button is hidden if the toolbar is too
narrow to accommodate it. An older prototype build will still jump directly
between wheel targets.

The existing plus and minus buttons continue to select traditional levels.

For a specific fractional magnification, enter:

```text
smoothzoom 1.3
```

To return the current viewport to integer zoom, use `smoothzoom 0`. To disable
smooth wheel controls, use `set gui.smooth_zoom 0`.

## Fly to a location

Flight navigation has a separate master switch, enabled by default in this fork. In **Game Options > Advanced**,
search for **fly** and enable **Fly-to navigation**. The master switch is visible
at Basic settings level; Expert settings expose the category overrides.
All destination categories default to on beneath the enabled master switch.
Use Expert settings only to opt out particular categories. Existing saved
configuration values remain in effect: enable any category you previously disabled.

Alternatively, enter `set gui.fly_navigation 1` in the console. It requires a
supported 32bpp blitter; the mouse-wheel smooth-zoom setting is independent.

Click a town in the town directory, or use a town window's location button.
The camera pans and zooms simultaneously along a continuous arc: it pulls back
while approaching the town, then moves in while continuing to approach.
Trips shorter than one starting viewport width avoid any extra pull-back.
This also applies to nearby industries: an eased pan without zoom is intentional.
For longer trips, no pull-back is possible when the extra zoom-out allowance is
zero or the view is already at its configured zoom-out limit.
Medium trips use the [van Wijk-Nuij zoom/pan model](https://vanwijk.win.tue.nl/zoompan.pdf),
which derives height and duration from the perceptual path. Animated journeys
last 300-2,200 ms. Longer journeys accelerate to finish within that limit; they
no longer silently switch to an instant jump after a previous town arrival. Disabling the existing smooth-scrolling setting also disables flights.
Flights now choose an arrival zoom for the destination, independently of the
departure zoom. Automatic fitting aims to keep the object within 80% of the
viewport and stops at a traditional zoom level. Small objects arrive at up to
200%; larger objects require a wider view. Your configured zoom limits apply.

Stations use their full spread, industries their footprint, and vehicles their
whole consist with surrounding context. Town fitting estimates their size from
the town influence radius. Terrain height and a roof allowance help frame
station and industry structures. Coordinate-only destinations use a normal
100% context view. These are framing estimates rather than exact sprite outlines.

The Expert **Town arrival zoom** override defaults to **Fit target automatically**;
fixed zoom and **Keep current zoom** remain available. Existing configurations
that selected Keep current zoom preserve that choice: select automatic fitting
or run `set gui.fly_town_zoom 0` to use the new default.

The following options tune the experience. Search for **flight** to find the
timing controls. Search for **arrival** to find the town zoom selector, or clear
the filter and expand **Graphics** to see all of these options together.

| Option | Default | Effect |
| --- | --- | --- |
| Fly-to navigation | On in this fork | Master switch for all navigation flights. |
| Fly to map locations | On, subject to master switch | Enables town-directory, town-location and uncategorised map-location flights, including console `scrollto`. Console setting: `gui.fly_towns`. |
| Flight speed | 120% | Perceptual path speed (1.2 by default); range 25-400%. Duration grows with path length rather than map distance. |
| Flight duration scale | 100% | Scale the whole journey; range 25-200%. |
| Flight curvature | 142 / 100 | Lower values give flatter flights, higher values pull back more; range 25-300 / 100. |
| Maximum extra flight zoom-out | 2 levels | Longer trips reveal more of the route, up to this allowance; range 0-3. Zero removes the extra pull-back. |
| Town arrival zoom | Fit target automatically | Override automatic fitting with a fixed level from 400% to 12.5%, or Keep current zoom. |

Every category has an Expert override, enabled by default. All require **Fly-to navigation**;
none requires **Fly to map locations** or another category to be enabled.

Expert settings provide independent category overrides; scroll for the remaining
options. All are enabled by default under the main switch.

| Switch | Navigation covered | Console setting |
| --- | --- | --- |
| Fly to vehicles | Vehicle window's location button | `gui.fly_vehicles` |
| Fly to stations | Station list and station location controls | `gui.fly_stations` |
| Fly to industries | Industry directory and industry location button | `gui.fly_industries` |
| Fly to news and events | News-item locations, including vehicle references | `gui.fly_news` |
| Fly to signs | Sign list and sign location controls | `gui.fly_signs` |
| Fly to smallmap destinations | Clicking a location on the smallmap | `gui.fly_smallmap` |
| Fly to company headquarters | Company's headquarters location control | `gui.fly_companies` |
| Fly to depots | Depot location controls | `gui.fly_depots` |
| Fly to waypoints | Waypoint location controls | `gui.fly_waypoints` |
| Fly to order destinations | Order-list and current-order destination controls | `gui.fly_orders` |
| Fly to scenario links | Story-page, goal and league-table location links | `gui.fly_scenarios` |
| Fly to subsidy destinations | Subsidy-list source/destination locations | `gui.fly_subsidies` |
| Fly to script destinations | Navigation requested by a game script | `gui.fly_scripts` |

For example, enable station flights without town or uncategorised map flights:

```text
set gui.fly_navigation 1
set gui.fly_towns 0
set gui.fly_stations 1
```

The switch follows the action used: a vehicle referenced in a news item uses
the news switch, while its own location button uses the vehicle switch. Order
destinations use the order switch even when the destination is a station.
The Expert **Fly to map locations** override combines town and uncategorised
map navigation; it does not override any other disabled category. The combined
switch retains `gui.fly_towns` and its existing value. The former
`gui.fly_other_navigation` setting is removed; use `gui.fly_towns` instead.

Turning off a category cancels any active flight in that category at its
currently displayed view. Turning off the master cancels every flight.
The vehicle window's **Centre <vehicle name> on location** button uses the
main switch by default; the Expert **Fly to vehicles** override can disable it.
Vehicle location flights (including vehicle news references) blend toward the live vehicle position during the final
20% of the journey, without extending the flight. A removed vehicle cancels the
flight. Double-click following remains immediate and tracks it continuously.

Flight speed, duration scale, curvature and maximum pull-back apply to all enabled
flights; changed values take effect on the next journey. Start with 120% speed,
100% duration and curvature 142. Faster speed or a smaller duration scale gives
quicker travel. Curvature is reduced automatically to fit zoom limits, while pan
and zoom stay simultaneous. The old `gui.fly_zoom_duration` setting is replaced
by `gui.fly_duration_scale` (percent). `gui.fly_curve` is curvature multiplied by 100.

Begin a viewport drag, scroll the wheel or press any key to
interrupt a flight and take control at the currently displayed position and
zoom. A drag cancels on gesture start, before its first movement. The cancelled
flight does not resume or jump to its former destination. The percentage button also stops
the flight and snaps to a traditional level. Selecting another destination
starts a new journey from the displayed view and fits the new destination.
With the explicit Keep current zoom override, retargeting preserves the intended
arrival zoom rather than the temporarily widened travel view. Ctrl-clicking a town still opens an extra
viewport, and explicit instant jumps and vehicle following stay immediate.
Disabling the master or the active flight's category, or changing the viewport
zoom limits, stops that flight at the current view, subject to those limits.

To try other destinations from the console:

```text
set gui.fly_navigation 1
set gui.fly_towns 1
scrollto 50 50
```

Leave the console open while that flight runs: closing it is a key press and
therefore interrupts the flight. `scrollto instant 50 50` bypasses the flight. Disable the feature with
`set gui.fly_navigation 0`. Flights are local display effects and do not change
the savegame format; loading cancels a pending journey.

## Zoom examples

These recordings show the current fractional rendering and panning behaviour.
They were captured after the sampling-phase fix included in published builds.
They illustrate functionality using actual game frames, not performance benchmarks.

![NewGRF objects and scaling labels during zoom](images/smooth-zoom/newgrf-objects.webp)

![Fractional pan and a stable resting view](images/smooth-zoom/fractional-pan-optimised.webp)

The object scene combines terrain, trees, NewGRF structures and map labels.
These clips do not demonstrate every vehicle type. The gallery uses general
scenes; individual vehicle checks are recorded in the
[coverage notes](validation/zoom-coverage.md), rather than separate movies.

See the [capture notes](images/smooth-zoom/README.md) for provenance and the
mixed-scene requirements for future recordings.

## Try the prototype

Inspect coastlines, slopes, roads and buildings while zooming and scrolling.
Look for tile seams, flickering, misplaced labels and sluggish rendering. Include
the zoom level and a screenshot when reporting a problem. Saving records a
nearby integer zoom level; loading resets the viewport to integer zoom.

See [the controls documentation](smooth_zoom_controls.md) for more behavior details.

### Fractional panning

Fractional views use the same fast sampler throughout camera movement and rest,
including wheel zoom and flight arrivals. There is no late area-filter handoff.
Native zoom levels retain their existing pixel-scroll path. No image cache is added.

For the earlier pan-performance fix, in the 960x600 software-rendered fixture at
130%, the same posted drag/capture
sequence observed a median interval between visible updates of 109 ms before
this change and 32 ms afterwards (worst gaps 133 ms and 88 ms). The land crop
remained unchanged beyond 300 ms after release. These are captured observations,
not an isolated performance benchmark: the baseline ran during compilation.
Larger resolutions, other renderers and complex NewGRFs need further checks.

The subsequent release-interval check found that changing filters could still
refine land after release. With consistent sampling, the static land crop's last
change was 87 ms after posting release, while final input/frame updates completed;
it then remained identical for the rest of the one-second observation. A native
snap and a subsequent wheel transition also passed the settling check.

### Map text and labels

Town names, station and waypoint labels, custom signs and their frames now
scale continuously with the map. Growth is deliberately gentler than terrain
scaling: labels stay readable on wide views and avoid becoming oversized close
up. Native and fractional zoom levels use the same sizing policy.

| Map zoom | Label size relative to the interface font |
| --- | --- |
| 400% | 141% |
| 200% | 119% |
| 100% | 100% |
| 50% | 84% |
| 25% and below | 75% minimum |

Their click rectangles and redraw bounds follow the displayed size. Normal map
labels keep the same font and content through zoom transitions, including town
population and station icons when enabled. Interface font settings still provide
the base size; toolbar and window text follow the interface size setting.
Legacy blitters without smooth zoom support retain their original labels.

The paused fixture was checked at 12.5%, 84%, 130%, 200% and 400%. Clicking a
station near its enlarged edge, a custom sign and a town name opened the expected
windows. Static labels and scenery were identical in captures 700 ms apart after
settling. The renderer reuses two scratch rasters for the current label rather
than caching every label, so text and colour updates take effect immediately.
Dense label overlap still follows the game's existing placement rules.

Graphics-set and renderer test results are recorded in the
[coverage notes](validation/zoom-coverage.md), including OpenGL, legacy blitters,
large windows, interface scaling and multilingual text. These notes distinguish
completed checks from work still in progress.

### Fractional panning

Panning now reuses the displayed pixels at a fixed fractional zoom, with the
same sampling phase for newly exposed scenery. This improves responsiveness
and avoids a later change when that scenery is redrawn. On the test machine,
130% panning improved from about 47 to 60 FPS at 960 × 600, and from about
16 to 58 FPS at 1920 × 1080. Changing the zoom still redraws the view.

The extended validation world also includes moving trams and NewGRF objects;
see the current zoom example above.
