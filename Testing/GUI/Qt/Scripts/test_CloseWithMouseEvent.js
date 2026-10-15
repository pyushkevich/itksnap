// Read the function library
include("Library");

// Closing the main image (Cmd-W with the main layer selected) unloads it at
// once, but the slice views only learn about it when they process the queued
// events. A mouse event in a slice view before that read the cursor of the
// unloaded data, and ITK-SNAP aborted with "Active segmentation not
// initialized". The mouse events are posted right after the close, so they
// arrive before the views have caught up.
openWorkspace("MRIcrop-with-mesh.itksnap");
engine.sleep(2000);

var canvas = engine.findChild(engine.findChild(mainwin, "panel0"), "sliceViewCanvas");
engine.trigger("actionUnload_Last_Overlay");
engine.postMouseEvent(canvas, 0.5, 0.5, "press", "left");
engine.postMouseEvent(canvas, 0.5, 0.5, "release", "left");
engine.sleep(2000);

//=== The main image is closed, and ITK-SNAP is still running
engine.validateValue(engine.getChildProperty(mainwin, "actionSaveSegmentation", "enabled"), false);
engine.print("No crash after closing the main image");
