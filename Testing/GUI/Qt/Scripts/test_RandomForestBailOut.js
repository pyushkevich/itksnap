// Read the function library
include("Library");

// Open the test workspace
openWorkspace("tensor.itksnap");

// Enter snake mode
enterSnakeModeFullROI();

//=== Entering classification mode
var snakepanel = engine.findChild(mainwin,"SnakeWizardPanel");
var combo = engine.findChild(snakepanel,"inPreprocessMode");
var index = engine.findItemRow(combo,"Classification");
engine.callMethod(combo, "setCurrentIndex", [index]);

//=== Show just the axial view
engine.clickChild(mainwin, "btnAxial");

// Get the axial panel
var panel0 = engine.findChild(mainwin,"panel0");
var sliceView0 = engine.findChild(panel0,"sliceView");

//=== Enter paintbrush mode
engine.trigger("actionPaintbrush");

//=== Paint with foreground label
setForegroundLabel("Label 1");
setCursor(20,8,13);
engine.postKeyEvent(sliceView0, "Space");

//=== Paint with background label
setForegroundLabel("Label 2");
setCursor(10,24,13);
engine.postKeyEvent(sliceView0, "Space");

//=== Paint with background label
setForegroundLabel("Label 3");
setCursor(26,28,13);
engine.postKeyEvent(sliceView0, "Space");

//=== Perform classification
engine.clickChild(snakepanel, "btnClassifyTrain");

//=== Cancel segmentation
engine.clickChild(snakepanel, "btnCancel");
engine.sleep(1000)
