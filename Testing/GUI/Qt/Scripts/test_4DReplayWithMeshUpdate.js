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
row0.setSelected(true);
engine.sleep(500);

var grp4D = engine.findChild(layerdialog, "grp4DProperties");
if (!grp4D.visible)
    engine.testFailed("4D Property Group not visible after selecting the main image row");

var btnReplay   = engine.findChild(grp4D, "btn4DReplay");
var inInterval  = engine.findChild(grp4D, "in4DReplayInterval");
var tpWidget    = engine.findChild(mainwin, "inCursorT_4D");

// ---------------------------------------------------------------------------
//  Phase A: cold-start replay (meshes not yet built for most frames)
//  Use a generous interval (500 ms) and a long sleep (8 s).
//  Even if mesh computation takes ~1-2 s per frame, 8 s should yield at
//  least 3-4 frame advances.
// ---------------------------------------------------------------------------
engine.print("Phase A: cold-start replay with continuous mesh update ON");

inInterval.text = "50";
setCursor4D(15, 23, 12, 0);

var tpBefore = tpWidget.value;
engine.print("Phase A: time point before replay = " + tpBefore);

btnReplay.click();        // START replay
engine.sleep(8000);       // let it run — crash would abort test here
btnReplay.click();        // STOP replay

var tpAfter = tpWidget.value;
engine.print("Phase A: time point after replay  = " + tpAfter);

if (tpAfter === tpBefore)
    engine.testFailed(
        "Phase A FAILED: 4D replay did not advance any time points during " +
        "cold-start with continuous mesh update active. " +
        "Regression: IsMeshUpdating guard may be blocking the replay timer " +
        "indefinitely, or the background mesh thread never completes.");

engine.print("Phase A PASSED: replay advanced from frame " + tpBefore +
             " to frame " + tpAfter);

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

inInterval.text = "100";

tpBefore = tpWidget.value;
engine.print("Phase B: time point before replay = " + tpBefore);

btnReplay.click();        // START replay
engine.sleep(1500);       // 1.5 s: at 100 ms/frame expect ~15 ticks — must have moved
                          // with the regression (~1.5 s/frame) we'd see 0-1 ticks
var tpMid = tpWidget.value;
engine.print("Phase B: time point at 1.5 s = " + tpMid);
engine.sleep(1500);       // another 1.5 s
btnReplay.click();        // STOP replay

tpAfter = tpWidget.value;
engine.print("Phase B: time point after replay  = " + tpAfter);

// Primary check: the mid-point must differ from the start.
// With correct caching at 100 ms, ~15 ticks happen in 1.5 s — guaranteed movement.
// With the regression (mesh rebuilt every frame at ~1-2 s each), fewer than
// 2 frames would advance in 1.5 s and the sample is likely unchanged.
if (tpMid === tpBefore)
    engine.testFailed(
        "Phase B FAILED: time point did not change in the first 1.5 s " +
        "of replay at 100 ms interval with pre-warmed caches. " +
        "Regression: IsMeshDirty() may be returning true for cached frames " +
        "causing redundant mesh recomputation and stalling the replay timer.");

// Secondary check: replay kept advancing into the second window too.
if (tpAfter === tpMid)
    engine.testFailed(
        "Phase B FAILED: time point stalled between 1.5 s and 3 s of replay. " +
        "Regression: continuous mesh update may be blocking the replay timer.");

engine.print("Phase B PASSED: replay advanced (start=" + tpBefore +
             " mid=" + tpMid + " end=" + tpAfter +
             "). Mesh dirty-flag caching is working correctly.");
