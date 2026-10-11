# Current demonstration clips

Only recordings of current behaviour are embedded in the documentation.
Older development clips and settings screenshots have been removed from those
pages, including alternate flight-speed and fixed-arrival examples. Speed and
arrival overrides remain available. Their old clips are not current evidence.

| Clip | Behaviour | Capture source |
| --- | --- | --- |
| `newgrf-objects.webp` | Fractional zoom and scaling labels | Sampling-phase fix, `7a4389b6c0` |
| `fractional-pan-optimised.webp` | Fractional panning and stable rest | Sampling-phase fix, `7a4389b6c0` |

Both clips were added with the final sampling-phase change. Subsequent camera
cleanup removed unused state without changing that rendering path. This fix is
included in release source `5bb7002e81fb`. These are representative demonstrations,
not exhaustive compatibility or performance checks.

Replace affected recordings when camera behaviour, controls or defaults change,
and record their capture revision here. A fresh flight or toolbar recording is
needed before either returns to the gallery. Do not restore earlier development
clips as examples of the current build.

## End-user gallery scope

Use a small set of general demonstrations, rather than one clip per vehicle or
object type. Individual vehicle close-ups, reproduction recordings, timing data
and renderer comparisons belong in ignored local validation output, not committed
end-user media. Keep written coverage results in the validation notes.

General zoom and flight recordings should show mixed scenes: terrain and water,
buildings and infrastructure, readable labels, and multiple moving vehicle types
where practical. Use complementary broad scenes when these cannot all fit in one
view. Captions must identify what is visible rather than imply every vehicle,
graphics set or NewGRF was tested by a single clip.

The retained object clip demonstrates terrain, trees, NewGRF structures and map
labels. It is not a mixed-vehicle demonstration. The next gallery refresh should
add a current-build overview containing road and rail traffic alongside stations,
bridges or tunnels, with waterways or an airport in a complementary broad view.
Do not bring back the separate bus, train, ship, aircraft or helicopter movies.

Keep the user guide text-only, with descriptive click-to-open links and clip
sizes. The homepage may embed one representative preview. Avoid embedding the
whole gallery in reading or installation pages.
