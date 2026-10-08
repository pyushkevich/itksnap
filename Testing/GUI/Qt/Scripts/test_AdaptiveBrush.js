// Read the function library
include("Library");

// Tests the adaptive (watershed) paintbrush. A single click with the adaptive
// brush paints the watershed region under the cursor, which is part of what a
// round brush of the same size would paint. When the active segmentation has
// a different grid from the image, the brush must fall back to plain painting.

openMainImage("MRIcrop-orig.gipl.gz");

//=== Show just the axial view
engine.clickChild(mainwin, "btnAxial");
engine.sleep(500);
var panel0 = engine.findChild(mainwin, "panel0");
var canvas = engine.findChild(panel0, "sliceViewCanvas");

//=== Paintbrush mode, a large round brush
engine.trigger("actionPaintbrush");
engine.sleep(500);
var brushpanel = engine.findChild(mainwin, "pagePaintbrushTool");
engine.clickChild(brushpanel, "btnRound");
engine.setChildProperty(brushpanel, "inBrushSizeSpinbox", "value", 15);
engine.sleep(500);

// Number of voxels with the given label in the active segmentation
function countVoxels(label)
{
    engine.trigger("actionVolumesAndStatistics");
    engine.sleep(1000);
    var dlg = engine.findWidget("dlgStatistics");
    engine.clickChild(dlg, "btnUpdate");
    engine.sleep(1000);
    var table = engine.findChild(dlg, "tvVolumes");
    var row = engine.findItemRow(table, label);
    var count = (row === undefined || row === null) ? 0
                : parseInt(String(engine.tableItemText(table, row, 1)).replace(/,/g, ""));
    engine.invoke(dlg, "close");
    engine.sleep(500);
    return count;
}

// A single click (no dragging) in the middle of the axial view
function clickCanvas()
{
    engine.postMouseEvent(canvas, 0.5, 0.5, "press", "left");
    engine.sleep(500);
    engine.postMouseEvent(canvas, 0.5, 0.5, "release", "left");
    engine.sleep(1000);
}

//=== Round brush click, then undo
engine.clickChild(brushpanel, "btnManual");
engine.sleep(500);
clickCanvas();
var n_round = countVoxels("Label 1");
engine.print("Round brush voxels: " + n_round);
engine.validateValue(n_round > 0, true);
engine.trigger("actionUndo");
engine.sleep(500);
engine.validateValue(countVoxels("Label 1"), 0);

//=== Adaptive brush click at the same place: paints part of the round brush
engine.clickChild(brushpanel, "btnWatershed");
engine.sleep(500);
clickCanvas();
var n_adaptive = countVoxels("Label 1");
engine.print("Adaptive brush voxels: " + n_adaptive);
engine.validateValue(n_adaptive > 0, true);
engine.validateValue(n_adaptive < n_round, true);
engine.trigger("actionUndo");
engine.sleep(500);

//=== Now make the active segmentation one with a different (0.4mm) grid,
//=== while the adaptive brush is still selected. The adaptive button is
//=== disabled, and a click paints with the plain brush instead.
openAdditionalSegmentation("MRIcrop-seg-hippoR-04mm.nii.gz");
engine.sleep(500);
engine.trigger("actionPaintbrush");
engine.sleep(500);
engine.validateChildProperty(brushpanel, "btnWatershed", "enabled", false);

// The segmentation covers only part of the image: put the cursor (and so the
// axial slice) inside it, and zoom the view to fit it so that the middle of
// the view is inside it. Zoom to fit cycles between the whole scene and the
// segmentation, so try each until a click lands inside the segmentation.
setCursor(31, 62, 40);
engine.sleep(500);
var n_fallback = 0;
for (var i = 0; i < 2 && n_fallback == 0; i++)
{
    engine.trigger("actionZoomToFitInAllViews");
    engine.sleep(500);
    clickCanvas();
    n_fallback = countVoxels("Label 1");
}
engine.print("Fallback brush voxels: " + n_fallback);
engine.validateValue(n_fallback > 0, true);
