// Read the function library
include("Library");

// Open the test workspace
openWorkspace("ultrasound_ws4d.itksnap");

// Open the Smoothing Dialog
engine.trigger("actionSmooth_Labels");

// Test label selection
var sDialog = engine.findChild(mainwin, "SmoothLabelsDialog");

engine.findChild(sDialog, "btnSelectAll").click();
engine.sleep(500);

engine.findChild(sDialog, "btnClearAll").click();
engine.sleep(500);

engine.findChild(sDialog, "btnSelectAll").click();

// Populate sigma values
engine.findChild(sDialog, "sigmaX").text = "1.2";
engine.findChild(sDialog, "sigmaY").text = "0.75";
engine.findChild(sDialog, "sigmaZ").text = "1.36";

// engine.sleep(500);

// Apply smoothing
engine.findChild(sDialog, "btnApply").click();

// engine.sleep(500);

var boxConfirm = engine.findWidget("boxConfirmSmoothing");
engine.invoke(boxConfirm, "accept");

engine.sleep(1000); // Pause for applying smoothing

// Test validation pop-up
engine.findChild(sDialog, "btnClearAll").click();
engine.findChild(sDialog, "btnApply").click();

// engine.sleep(500);
