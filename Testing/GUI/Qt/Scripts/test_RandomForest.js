// Read the function library
thread.source("Library");

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

//=== Clear segmentation
engine.findChild(snakepanel,"btnClassifyClearExamples").click();

//=== Go to bubble mode
engine.findChild(snakepanel,"btnNextPreproc").click();
thread.wait(1000);

//=== Go back to the first seed 
setCursor(20,8,13);

//=== Validating speed image
engine.validateFloatValue(readVoxelIntensity(4), 0.8, 0.4)

//=== Add a bubble
engine.findChild(snakepanel,"btnAddBubble").click();

//=== Set bubble radius
engine.findChild(snakepanel,"inBubbleRadius").value = 4;

//=== Go to snake mode
engine.findChild(snakepanel,"btnBubbleNext").click();
thread.wait(1000);

//=== Set step size
engine.findChild(snakepanel,"inStepSize").value = 10;

//=== Run snake 100 iter
for(var i = 0; i < 10; i++)
{
  engine.findChild(snakepanel,"btnSingleStep").click();
  
}

//=== Finish snake mode
engine.findChild(snakepanel,"btnEvolutionNext").click()
thread.wait(1000)

//=== Open volumes and statistics
engine.findChild(mainwin,"actionVolumesAndStatistics").trigger();

//=== Check the value
var dialog = engine.findChild(mainwin, "dlgStatistics");
var table =  engine.findChild(dialog, "tvVolumes");
var value =  engine.tableItemText(table,1,1);
engine.validateFloatValue(value, 1200, 600);

