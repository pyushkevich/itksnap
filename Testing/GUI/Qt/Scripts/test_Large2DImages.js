// Read the function library
include("Library");

// Tests large 2D images (1280 x 640 x 1, made by TestData/make_large2d.py).
// Images wider than 1024 voxels are always sliced non-orthogonally. Checks that
// a 2D image opens with only its 2D view shown, that the main image, overlays
// and segmentation show the right values under the cursor, and that the
// paintbrush works, including the adaptive brush (a large image is not rotated). The main image is a pattern of 128x128 blocks with values
// 20 + (7 * bx + 13 * by) % 200; overlays hold 255 minus the main image.

// Visibility of the four view panels, e.g. "0100"
function visiblePanels()
{
    var s = "";
    ["panel0", "panel1", "panel2", "panel3D"].forEach(function(name) {
        s += engine.getProperty(engine.findChild(mainwin, name), "visible") ? "1" : "0";
    });
    return s;
}

// Exactly one of the three slice views is shown, and the 3D view is hidden
function checkSingle2DView(what)
{
    var vis = visiblePanels();
    engine.print(what + ": visible panels " + vis);
    var n = 0;
    for (var i = 0; i < 3; i++)
        if (vis[i] == "1")
            n++;
    if (n != 1 || vis[3] != "0")
        engine.testFailed(what + ": expected only the 2D view, got panels " + vis);
}

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

//=== A three-component image (issue #200: large 2D color images did not render)
openMainImage("large2d_rgb.nii.gz");
engine.sleep(1000);
checkSingle2DView("RGB image");

// Block (2, 1): R = 47
setCursor(301, 201, 1);
engine.validateValue(readVoxelIntensity(0), 47);

//=== The grayscale image
openMainImage("large2d_main.nii.gz");
engine.sleep(1000);
checkSingle2DView("Grayscale image");

//=== A segmentation on the same grid, and two overlays: one whose plane is
//=== 0.3 voxel off along z, and one at half the resolution
openSegmentation("large2d_seg.nii.gz");
engine.sleep(500);
openOverlay("large2d_ov_zshift.nii.gz");
engine.sleep(500);
openOverlay("large2d_ov_halfres.nii.gz");
engine.sleep(1000);
checkSingle2DView("With overlays");

// Inside block (2, 1), which is in the label 1 rectangle (x 256-511, y 128-255)
setCursor(301, 201, 1);
engine.validateValue(readVoxelIntensity(0), 47);
engine.validateValue(readVoxelIntensity(1), 208);
engine.validateValue(readVoxelIntensity(2), 208);
engine.validateValue(engine.getChildProperty(mainwin, "outLabelId", "value"), 1);

// Inside block (6, 3), in the label 2 rectangle (x 768-1023, y 384-511)
setCursor(801, 401, 1);
engine.validateValue(readVoxelIntensity(0), 20 + (7 * 6 + 13 * 3) % 200);
engine.validateValue(readVoxelIntensity(1), 255 - (20 + (7 * 6 + 13 * 3) % 200));
engine.validateValue(engine.getChildProperty(mainwin, "outLabelId", "value"), 2);

// The first voxel: the overlays are sampled at their edge, and must not be dimmed
setCursor(1, 1, 1);
engine.validateValue(readVoxelIntensity(0), 20);
engine.validateValue(readVoxelIntensity(1), 235);
engine.validateValue(readVoxelIntensity(2), 235);
engine.validateValue(engine.getChildProperty(mainwin, "outLabelId", "value"), 0);

//=== Paint with a round brush in the middle of the 2D view (outside both
//=== rectangles), then undo
var n_before = countVoxels("Label 1");
engine.validateValue(n_before, 256 * 128);

var panel = engine.findChild(mainwin, "panel1");
if (!engine.getProperty(panel, "visible"))
    panel = engine.findChild(mainwin, visiblePanels()[0] == "1" ? "panel0" : "panel2");
var canvas = engine.findChild(panel, "sliceViewCanvas");

engine.trigger("actionPaintbrush");
engine.sleep(500);
var brushpanel = engine.findChild(mainwin, "pagePaintbrushTool");
engine.clickChild(brushpanel, "btnRound");
engine.setChildProperty(brushpanel, "inBrushSizeSpinbox", "value", 15);
engine.sleep(500);

engine.postMouseEvent(canvas, 0.5, 0.5, "press", "left");
engine.sleep(500);
engine.postMouseEvent(canvas, 0.5, 0.5, "release", "left");
engine.sleep(1000);

// A 2D brush of diameter 15 paints about pi * 7.5^2 = 177 voxels
var n_painted = countVoxels("Label 1") - n_before;
engine.print("Painted voxels: " + n_painted);
engine.validateValue(n_painted > 100 && n_painted < 260, true);

engine.trigger("actionUndo");
engine.sleep(500);
engine.validateValue(countVoxels("Label 1"), n_before);

//=== The adaptive (watershed) brush is available: the image is large, but not
//=== rotated. A click paints part of the watershed region of the 128x128 block
//=== under the brush, which is at most what the round brush paints.
engine.validateChildProperty(brushpanel, "btnWatershed", "enabled", true);
engine.clickChild(brushpanel, "btnWatershed");
engine.sleep(500);
engine.postMouseEvent(canvas, 0.5, 0.5, "press", "left");
engine.sleep(500);
engine.postMouseEvent(canvas, 0.5, 0.5, "release", "left");
engine.sleep(1000);
var n_adaptive = countVoxels("Label 1") - n_before;
engine.print("Adaptive brush voxels: " + n_adaptive);
engine.validateValue(n_adaptive > 0 && n_adaptive <= n_painted, true);
engine.trigger("actionUndo");
engine.sleep(500);
engine.validateValue(countVoxels("Label 1"), n_before);
