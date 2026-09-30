// Read the function library
include("Library");

// Open the test workspace
openWorkspace("ultrasound_ws4d.itksnap");

// Open the Smoothing Dialog
engine.trigger("actionSmooth_Labels");

// Test label selection
var sDialog = engine.findChild(mainwin, "SmoothLabelsDialog");

engine.clickChild(sDialog, "btnSelectAll");
engine.sleep(500);

engine.clickChild(sDialog, "btnClearAll");
engine.sleep(500);

engine.clickChild(sDialog, "btnSelectAll");

// Populate sigma values
engine.setChildProperty(sDialog, "sigmaX", "text", "1.2");
engine.setChildProperty(sDialog, "sigmaY", "text", "0.75");
engine.setChildProperty(sDialog, "sigmaZ", "text", "1.36");

// engine.sleep(500);

// Apply smoothing
engine.clickChild(sDialog, "btnApply");

// engine.sleep(500);

var boxConfirm = engine.findWidget("boxConfirmSmoothing");
engine.invoke(boxConfirm, "accept");

engine.sleep(1000); // Pause for applying smoothing

// Test validation pop-up
engine.clickChild(sDialog, "btnClearAll");
engine.clickChild(sDialog, "btnApply");

// engine.sleep(500);
