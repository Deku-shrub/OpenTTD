# Performance and QA

Demo capture is paused while performance and correctness are checked. Use the
downloaded working network described in [capture provenance](promotional-captures.md).
Keep raw logs and results in ignored `build-smoothzoom/performance` output.

## Comparisons

Compare smooth zoom enabled and disabled at the same traditional zoom, map
location, client resolution and traffic state. Measure idle drawing, continuous
panning and complete redraws separately. At fractional levels, compare nearby
traditional scales and record the field-of-view difference. Dense-map cost
already present with the feature disabled is acceptable. Additional rendering
cost from the feature remains an optimization target.

Use the same executable for on/off comparisons. Compare candidate and retained
executables with the same save and input sequence. Record executable and save
hashes. Repeat timings and use native rendering as a control for timing drift.

The optional production-blitter benchmark covers integer drawing, nearest
sampling and the retained area-filter comparison. It includes SSE4 where the
CPU supports it, animated palette colours, terrain alpha and scales on both
sides of native mipmap boundaries. Its inner-loop timings exclude viewport
construction, presentation and game simulation.

```powershell
C:/ottd-build/Release/openttd_test.exe "[smoothzoom-benchmark]"
```

## Real-map runs

`run-zoom-performance.py` launches its own isolated Windows game. It stages
OpenGFX 8, generated NoSound/NoMusic descriptors and a matching language file.
Pillow and staged base resources are shared with `run-zoom-matrix.py`.

```powershell
python docs/validation/run-zoom-performance.py `
  --executable C:/ottd-build/Release/openttd.exe `
  --save build-smoothzoom/promo/profile/save/promo-base.sav `
  --out build-smoothzoom/performance/run-name `
  --group all --rounds 2 --seconds 8 --cycles 32
```

Choose a new output directory for each run. The groups are:

- `baseline`: native and fractional idle drawing, continuous panning and full
  redraws, followed by paused/running rail and airport comparisons.
- `interaction`: slow and rapid wheel input, reversal, short and long
  navigation, and manual pan/zoom takeover. Flight traces must confirm enabled
  flights arrive and takeover aborts them. Disabled navigation must create no
  flight trace.
- `soak`: repeated zoom-out/in and pan cycles, with memory samples each cycle
  and matched fractional pan checks before and after the sequence.

Use `--actions redraw --group baseline` for a focused redraw comparison.
`--no-labels` and `--no-animation` isolate label and palette-animation cost.
`--size 1280 720` changes client resolution. Resolution comparisons also change
visible map area at a fixed scale. `--opengl` selects the OpenGL/40bpp renderer.

The isolated profile binds `print_framerate` to F6. This otherwise unbound
shortcut writes the existing timing statistics without opening the console
over the measured viewport. `hide_console` lets navigation scripts close the
console before starting a flight. Return is sent once, relying on Windows'
character translation. Duplicate synthetic Return events would abort flights.
Timing runs use debug level 5 for camera summaries, excluding unrelated verbose
order checks.

The script sends messages only to the game it launches. Foreground runs stop
if that game loses focus. Owned modal dialogs stop input and are recorded as
failures. Invalid startup runs are excluded from timing comparisons.

With `--background`, the script can measure viewport CPU cost and process
memory while the desktop is locked or another application is foreground.
These results are labelled background diagnostics. Foreground presentation and
perceived camera smoothness require a separate unlocked run. Runs produce logs
and JSON, without desktop or video recording. `--group follow --vehicle-probe`
saves one owned-game setup screenshot outside a timed action. Use it to locate
the vehicle's centre button, then supply `--vehicle-location <x> <y>` for the
following tests at 100%, 130% and 33.33%. They assert that following is active.

The console reports average processing durations for its most recent 64, 128
and 512 samples. Steady-state cases use the recent 64-sample value. Zoom and
flight cadence comes from camera traces, since post-action console averages can
include settled frames. Private bytes and working set are recorded before and
after each action. Initial cache growth needs to be distinguished from continued
growth after repeated warm cycles.

## Pending coverage

- Longer foreground repetitions and a foreground repeated-use run. The initial interaction and dense-baseline runs completed.
- Repeat the slower OpenGL fractional-redraw case and profile its rendering cost.
- Investigate continued small private-memory growth in the paused repeated-use run.
- Review long frame intervals and arrival stability in the representative
  network before resuming promotional capture.

Current measurements and renderer checks are in [coverage notes](zoom-coverage.md).
