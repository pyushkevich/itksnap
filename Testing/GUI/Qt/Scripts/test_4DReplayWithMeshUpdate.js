// Read the function library
include("Library");

// ---------------------------------------------------------------------------
// Regression test for two related bug classes:
//
//  1. SIGSEGV / SIGABRT crash when 4D replay runs concurrently with
//     continuous mesh update (data race on the ITK pipeline between the
//     main-thread replay timer and the background mesh computation thread).
//
//  2. SegmentationMeshWrapper::IsMeshDirty() returning true every frame
//     because GetImage() re-runs the time-point-select filter and bumps
//     the image MTime, causing the mesh to be redundantly recomputed for
//     time points that have already been meshed.
//
// Strategy
// --------
//  Phase A – cold start:
//    Enable continuous update, start 4D replay immediately (meshes not yet
//    built).  Let it run for several seconds.  Verify the cursor time point
//    actually advanced — a stuck IsMeshUpdating guard would prevent any
//    advancement, while a crash would abort the test entirely.
//
//  Phase B – pre-warmed cache:
//    Scroll manually through all 11 time points so every frame's mesh is
//    built and cached.  Then start replay again.  With a correct dirty-flag
//    implementation the cached meshes are reused and replay advances at the
//    set interval; with the regression replay stalls while the pipeline
//    needlessly rebuilds every frame.  We verify at least N frames advanced
//    within a fixed time budget that would be too tight if computation were
//    required every frame.
// ---------------------------------------------------------------------------

//=== Open the 4D workspace that has segmentation layers on 11 time points
openWorkspace("segmentation_mesh.itksnap");

//=== Enable continuous mesh update
engine.trigger("actionContinuous_Update");
engine.sleep(500);

//=== Open the layer inspector to access 4D replay controls
engine.trigger("actionLayerInspector");
var layerdialog = engine.findChild(mainwin, "dlgLayerInspector");

// Select the main image row (row 0) so the 4D property group becomes visible
var row0 = engine.findChild(layerdialog, "wgtRowDelegate_0000");
engine.setProperty(row0, "selected", true);
engine.sleep(500);

var grp4D = engine.findChild(layerdialog, "grp4DProperties");
if (!engine.getProperty(grp4D, "visible"))
    engine.testFailed("4D Property Group not visible after selecting the main image row");

var btnReplay   = engine.findChild(grp4D, "btn4DReplay");
var inInterval  = engine.findChild(grp4D, "in4DReplayInterval");
var tpWidget    = engine.findChild(mainwin, "inCursorT_4D");

// Watch the time point while replay runs for duration_ms, reading it every
// 250 ms, and return how many readings differed from the one before. Replay
// cycles through the 11 frames, so comparing only the first and last frame
// can fail by chance when replay ends on the frame it started from. 250 ms
// is not a multiple of the replay intervals used below (50 and 100 ms), so
// the readings cannot keep landing on the same frame either.
function countFrameChanges(duration_ms)
{
    var step = 250;
    var prev = engine.getProperty(tpWidget, "value");
    var changes = 0;
    for (var t = 0; t < duration_ms; t += step)
    {
        engine.sleep(step);
        var tp = engine.getProperty(tpWidget, "value");
        if (tp !== prev)
            changes++;
        prev = tp;
    }
    return changes;
}

// ---------------------------------------------------------------------------
//  Phase A: cold-start replay (meshes not yet built for most frames)
//  Use a generous interval (500 ms) and a long sleep (8 s).
//  Even if mesh computation takes ~1-2 s per frame, 8 s should yield at
//  least 3-4 frame advances.
// ---------------------------------------------------------------------------
engine.print("Phase A: cold-start replay with continuous mesh update ON");

engine.setProperty(inInterval, "text", "50");
setCursor4D(15, 23, 12, 0);

var tpBefore = engine.getProperty(tpWidget, "value");
engine.print("Phase A: time point before replay = " + tpBefore);

engine.click(btnReplay);   // START replay
var changesA = countFrameChanges(8000);  // let it run — crash would abort test here
engine.click(btnReplay);   // STOP replay

var tpAfter = engine.getProperty(tpWidget, "value");
engine.print("Phase A: time point after replay  = " + tpAfter +
             ", frame changes seen in 8 s = " + changesA);

if (changesA < 1)
    engine.testFailed(
        "Phase A FAILED: 4D replay did not advance any time points during " +
        "cold-start with continuous mesh update active. " +
        "Regression: IsMeshUpdating guard may be blocking the replay timer " +
        "indefinitely, or the background mesh thread never completes.");

engine.print("Phase A PASSED: replay advanced " + changesA + " times");

// ---------------------------------------------------------------------------
//  Phase B: pre-warm all 11 frame mesh caches, then verify replay runs
//  at close to the set interval (no redundant recomputation).
//
//  Each setCursor4D call includes a 200 ms sleep; we add an extra 1800 ms
//  to let the background thread finish building the mesh before moving on.
// ---------------------------------------------------------------------------
engine.print("Phase B: pre-building meshes for all 11 time points");

for (var i = 0; i <= 10; i++) {
    setCursor4D(15, 23, 12, i);
    engine.sleep(1800);   // wait for background mesh build to complete
}

// Return to frame 0 to get a clean starting position
setCursor4D(15, 23, 12, 0);
engine.sleep(500);

engine.print("Phase B: starting replay over pre-warmed cache at 100 ms interval");

engine.setProperty(inInterval, "text", "100");

tpBefore = engine.getProperty(tpWidget, "value");
engine.print("Phase B: time point before replay = " + tpBefore);

engine.click(btnReplay);   // START replay
// At 100 ms/frame, each 250 ms reading sees the frame change, so about 6 of
// the 6 readings in 1.5 s differ. With the regression (~1.5 s/frame) at most
// 1-2 would. Two windows: replay must start fast and keep going.
var changesB1 = countFrameChanges(1500);
engine.print("Phase B: frame changes seen in the first 1.5 s = " + changesB1);
var changesB2 = countFrameChanges(1500);
engine.print("Phase B: frame changes seen in the next 1.5 s  = " + changesB2);
engine.click(btnReplay);   // STOP replay

if (changesB1 < 3)
    engine.testFailed(
        "Phase B FAILED: only " + changesB1 + " frame changes in the first 1.5 s " +
        "of replay at 100 ms interval with pre-warmed caches. " +
        "Regression: IsMeshDirty() may be returning true for cached frames " +
        "causing redundant mesh recomputation and stalling the replay timer.");

if (changesB2 < 3)
    engine.testFailed(
        "Phase B FAILED: only " + changesB2 + " frame changes between 1.5 s and 3 s " +
        "of replay. Regression: continuous mesh update may be blocking the replay timer.");

engine.print("Phase B PASSED: replay advanced " + changesB1 + " + " + changesB2 +
             " times. Mesh dirty-flag caching is working correctly.");
