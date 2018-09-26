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
combo.setCurrentIndex(index);

//=== Show just the axial view
engine.findChild(mainwin, "btnAxial").click();

// Get the axial panel
var panel0 = engine.findChild(mainwin,"panel0");
var sliceView0 = engine.findChild(panel0,"sliceView");

//=== Enter paintbrush mode
engine.findChild(mainwin,"actionPaintbrush").trigger();

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
engine.findChild(snakepanel,"btnClassifyTrain").click();

//=== Cancel segmentation
engine.findChild(snakepanel, "btnCancel").click();
engine.sleep(1000)
