# Smooth zoom controls

Smooth zoom is an optional client graphics setting, enabled by default in this fork. It
does not change network game settings, and is stored in the local configuration
as `gui.smooth_zoom`. Find Smooth mouse-wheel zoom in Game Options > Advanced,
select Expert / all settings, and search for smooth. It is not in
Game Options > Graphics. You can also enable it in the console with
`set gui.smooth_zoom 1`. See [setup and use](smooth_zoom_readme.md) for the prepared
Windows prototype launch command.

With the setting enabled and a supported 32bpp blitter active, the wheel selects
targets in four steps per doubling or halving. Motion eases towards the target,
including when the simulation is paused. Repeated wheel input extends the
target while preserving the current velocity, so a continuous gesture does
not restart an easing curve at every notch. Main and extra viewports
keep the world position under the cursor fixed. A main viewport following a
vehicle and the viewport inside a vehicle window instead keep their centre
fixed, preserving vehicle following. An unsupported blitter retains the usual
integer wheel zoom.

The client's maximum zoom-in and zoom-out settings apply to fractional zoom.
Zoom buttons and keyboard zoom shortcuts move to the next integer level in the
requested direction. Exact integer levels return to the existing integer
rendering path. Disabling the setting rounds active fractional viewports to
their nearest permitted integer level and preserves their centre.

The main toolbar shows a compact percentage button, for example 84%.
Its tooltip shows the nearest traditional level, such as 100%. Click the
button to stop an active transition and snap to that traditional level,
preserving the viewed centre. The existing +/- buttons select adjacent levels.
The readout appears beside the zoom buttons when there is room; at very narrow
widths, the existing split toolbar keeps its zoom buttons available.

Dragging, keyboard arrow scrolling and scrolling near viewport edges use the
actual fractional scale. Resizing recomputes the viewport's virtual dimensions
from that same scale. Extra viewport Copy Location and Paste Location controls
use the virtual dimensions, so they preserve the world centre even when the two
viewports use different fractional scales.

Savegames retain the existing integer zoom format. Saving a fractional viewport
records its nearest integer level and adjusts the position to preserve its
centre. Loading resets fractional zoom; the client setting remains
available for subsequent wheel events. This requires no savegame format change.

## Verification

The `[smoothzoom]` test cases in `src/tests/smooth_zoom_controls.cpp` exercise the
same numerical helpers used by the controls. They check that four wheel steps
span one integer level, zoom-in and zoom-out reverse each other, restricted zoom
limits clamp correctly, cursor anchoring changes the world position by at most
half a virtual pixel, and buttons select adjacent integer levels rather than
skipping an integer level when starting from a fractional scale.

Desktop integration checks should also cover:

- Enable the setting, zoom at an identifiable land tile and confirm that tile
  stays under the cursor. Test near both zoom limits and click the zoom buttons.
- Disable the setting at a fractional level and confirm integer controls resume.
- Resize, drag, use arrow keys and enable edge scrolling at a fractional level.
- Open an extra viewport, use Copy Location and Paste Location at different
  scales, then resize and scroll the extra viewport.
- Open a vehicle window, wheel-zoom its viewport and verify the vehicle remains
  centred. Follow a vehicle in the main viewport and repeat.
- Save at a fractional scale and reload; check that the viewed centre is retained
  and wheel zoom remains available.
- Switch between supported 32bpp rendering and an unsupported blitter, confirming
  that integer zoom remains usable.

Map labels scale gently with zoom at native and fractional levels: 100% map zoom uses the interface font size, 400% uses about 141%, and wide views stop shrinking at 75%. Their frames and click targets scale with the text. Window and toolbar text continue to use the interface size setting. See the [label examples](smooth_zoom_readme.md#map-text-and-labels).
