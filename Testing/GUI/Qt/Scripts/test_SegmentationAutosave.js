// Read the function library
include("Library");

// Tests the autosave of segmentations with unsaved changes (for recovery
// after a crash). The timer doesn't run in test mode, so the test calls
// autosaveNow() itself. A layer is written only when it changed since the
// last autosave, the autosave doesn't mark the layer as saved, the file is
// dropped once the user saves the layer, and the file holds the segmentation.
// Last, the save that the crash handler makes writes the latest state and
// turns the session folder into a recovery folder.

openMainImage("MRIcrop-orig.gipl.gz");

var autosave = engine.findChild(mainwin, "segmentationAutosave");
engine.setProperty(autosave, "sessionDir", tempdir + "/SegmentationAutosave_" + Date.now());

// Autosave now and wait for the write to finish
function autosaveNow()
{
    engine.callMethod(autosave, "autosaveNow", [], true);
    for (var i = 0; i < 100 && engine.getProperty(autosave, "writing"); i++)
        engine.sleep(100);
    engine.validateValue(engine.getProperty(autosave, "writing"), false);
}

//=== Show just the axial view, with a large round brush
engine.clickChild(mainwin, "btnAxial");
engine.sleep(500);
var panel0 = engine.findChild(mainwin, "panel0");
var canvas = engine.findChild(panel0, "sliceViewCanvas");
engine.trigger("actionPaintbrush");
engine.sleep(500);
var brushpanel = engine.findChild(mainwin, "pagePaintbrushTool");
engine.clickChild(brushpanel, "btnRound");
engine.clickChild(brushpanel, "btnManual");
engine.setChildProperty(brushpanel, "inBrushSizeSpinbox", "value", 15);
engine.sleep(500);

function paintAt(x, y)
{
    engine.postMouseEvent(canvas, x, y, "press", "left");
    engine.sleep(500);
    engine.postMouseEvent(canvas, x, y, "release", "left");
    engine.sleep(1000);
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

//=== Nothing to save before painting
engine.validateValue(engine.getProperty(autosave, "unsavedLayerCount"), 0);
autosaveNow();
engine.validateValue(engine.getProperty(autosave, "savedLayerCount"), 0);

//=== Paint and autosave. The layer still has unsaved changes afterwards.
paintAt(0.5, 0.5);
engine.validateValue(engine.getProperty(autosave, "unsavedLayerCount"), 1);
autosaveNow();
engine.validateValue(engine.getProperty(autosave, "savedLayerCount"), 1);
engine.validateValue(engine.getProperty(autosave, "writeCount"), 1);
engine.validateValue(engine.getProperty(autosave, "unsavedLayerCount"), 1);

//=== Without changes, the layer isn't written again
autosaveNow();
engine.validateValue(engine.getProperty(autosave, "writeCount"), 1);

//=== After a change, it is
paintAt(0.3, 0.3);
autosaveNow();
engine.validateValue(engine.getProperty(autosave, "writeCount"), 2);

//=== Once the user saves the layer, its autosave file is dropped
engine.trigger("actionSaveSegmentation");
engine.sleep(2000);
var dialog = engine.findChild(mainwin, "wizImageIO");
engine.setChildProperty(dialog, "inFilename", "text", tempdir + "/SegmentationAutosave_seg.nii.gz");
engine.clickChild(dialog, "qt_wizard_finish");
engine.sleep(2000);
engine.validateValue(engine.getProperty(autosave, "unsavedLayerCount"), 0);
autosaveNow();
engine.validateValue(engine.getProperty(autosave, "savedLayerCount"), 0);

//=== Paint again and autosave. The file holds the painted segmentation.
paintAt(0.7, 0.6);
autosaveNow();
engine.validateValue(engine.getProperty(autosave, "savedLayerCount"), 1);
var n_painted = countVoxels("Label 1");
engine.print("Painted voxels: " + n_painted);
engine.validateValue(n_painted > 0, true);

// Load a segmentation from an absolute path as an additional layer
function openSegmentationFile(filename)
{
    engine.trigger("actionAddSegmentation_Open");
    engine.sleep(2000);
    var dlg = engine.findChild(mainwin, "wizImageIO");
    engine.setChildProperty(dlg, "inFilename", "text", filename);
    engine.clickChild(dlg, "qt_wizard_commit");
    engine.sleep(1000);
    engine.clickChild(dlg, "qt_wizard_finish");
    engine.sleep(2500);
}

//=== Paint once more without autosaving, then make the crash handler's save.
//=== It writes the latest state and renames the folder to Recovery_<time>.
var saved = String(engine.getProperty(autosave, "savedFiles")[0]);
var saved_name = saved.substring(saved.lastIndexOf("/") + 1);
paintAt(0.4, 0.7);
var n_latest = countVoxels("Label 1");
engine.print("Painted voxels before the crash save: " + n_latest);
engine.validateValue(n_latest > n_painted, true);

engine.callMethod(autosave, "SaveForCrash", [], true);
var recovery_dir = String(engine.getProperty(autosave, "sessionDir"));
engine.print("Recovery folder: " + recovery_dir);
engine.validateValue(recovery_dir.substring(recovery_dir.lastIndexOf("/") + 1).startsWith("Recovery_"), true);
engine.validateValue(engine.getProperty(autosave, "savedLayerCount"), 0);

openSegmentationFile(recovery_dir + "/" + saved_name);
engine.validateValue(countVoxels("Label 1"), n_latest);
