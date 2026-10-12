// Read the function library
include("Library");

// The active segmentation (which defines the reference space) is saved in the
// workspace. ITK-SNAP is started with a workspace made by itksnap-wt (see
// CMakeLists.txt): main image, MRIcrop-seg (same grid as the main image) and
// the cropped, upsampled hippo-R segmentation, which is marked as active
// with -props-set-active-seg.
waitForMainImage();
engine.sleep(1000);

// The scene is measured in voxels of the active segmentation: the main
// image is 78 voxels wide, but 195 voxels of the 0.4mm hippo-R segmentation
function sceneWidth()
{
    return engine.getChildProperty(mainwin, "inCursorX", "maximum")
         - engine.getChildProperty(mainwin, "inCursorX", "minimum") + 1;
}

// Save or open a workspace through the file dialog. The dialog is kept after
// it is closed, so look for the visible one.
function workspaceFileDialog(action, file)
{
    engine.trigger(action);
    engine.sleep(2000);
    var dlg = engine.findVisibleChild(mainwin, "dlgSimpleFile");
    if (!dlg)
        engine.testFailed("No file dialog shown for " + action);
    engine.setChildProperty(dlg, "inFilename", "text", file);
    engine.invoke(dlg, "accept");
    engine.sleep(3000);
}

//=== hippo-R (the last layer, marked by itksnap-wt) is active, not the first
engine.validateValue(sceneWidth(), 195, 2);

//=== Save one workspace with MRIcrop-seg active and one with hippo-R active
var ws_first = tempdir + "/WorkspaceActiveSegmentation_first.itksnap";
var ws_last = tempdir + "/WorkspaceActiveSegmentation_last.itksnap";

engine.trigger("actionActivatePreviousSegmentationLayer");
engine.sleep(500);
engine.validateValue(sceneWidth(), 78);
workspaceFileDialog("actionSaveWorkspaceAs", ws_first);

engine.trigger("actionActivateNextSegmentationLayer");
engine.sleep(500);
engine.validateValue(sceneWidth(), 195, 2);
workspaceFileDialog("actionSaveWorkspaceAs", ws_last);

//=== Each workspace reopens with the segmentation that was active when it
//=== was saved
workspaceFileDialog("actionOpenWorkspace", ws_first);
waitForMainImage();
engine.validateValue(sceneWidth(), 78);

workspaceFileDialog("actionOpenWorkspace", ws_last);
waitForMainImage();
engine.validateValue(sceneWidth(), 195, 2);

//=== Both segmentations were reloaded
engine.trigger("actionActivatePreviousSegmentationLayer");
engine.sleep(500);
engine.validateValue(sceneWidth(), 78);
