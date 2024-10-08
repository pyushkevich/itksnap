// Read the function library
include("Library");

// Open the test workspace
openWorkspace("tensor.itksnap");

// Enter snake mode
enterSnakeModeFullROI();

//=== Entering classification mode
var snakepanel = engine.findChild(mainwin,"SnakeWizardPanel");
var combo = engine.findChild(snakepanel,"inPreprocessMode");
engine.comboBoxSelect(combo, "Classification");

//=== Show just the axial view
engine.findChild(mainwin, "btnAxial").click();

// Get the axial panel
var panel0 = engine.findChild(mainwin,"panel0");
var sliceViewInternalWidget0 = engine.findChild(engine.findChild(panel0, "sliceView"), "internalWidget");

//=== Enter paintbrush mode
engine.trigger("actionPaintbrush");

//=== Paint with foreground label
setForegroundLabel("Label 1");
setCursor(20,8,13);
engine.postKeyEvent(sliceViewInternalWidget0, "Space");

//=== Paint with background label
setForegroundLabel("Label 2");
setCursor(10,24,13);
engine.postKeyEvent(sliceViewInternalWidget0, "Space");

//=== Paint with background label
setForegroundLabel("Label 3");
setCursor(26,28,13);
engine.postKeyEvent(sliceViewInternalWidget0, "Space");

//=== Perform classification
engine.findChild(snakepanel,"btnClassifyTrain").click();

//=== Clear segmentation
engine.findChild(snakepanel,"btnClassifyClearExamples").click();

//=== Go to bubble mode
engine.findChild(snakepanel,"btnNextPreproc").click();
engine.sleep(1000);

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
engine.sleep(1000);

//=== Set step size
engine.findChild(snakepanel,"inStepSize").value = 10;

//=== Turn on continuous rendering
engine.trigger("actionContinuous_Update");

//=== Run snake 100 iter
for(var i = 0; i < 10; i++)
{
  engine.findChild(snakepanel,"btnSingleStep").click();
  
}

//=== Finish snake mode
engine.findChild(snakepanel,"btnEvolutionNext").click()
engine.sleep(1000)

//=== Open volumes and statistics
engine.trigger("actionVolumesAndStatistics");

//=== Check the value
var dialog = engine.findChild(mainwin, "dlgStatistics");
var table =  engine.findChild(dialog, "tvVolumes");
var value =  engine.tableItemText(table,1,1);
engine.validateFloatValue(value, 1200, 600);

