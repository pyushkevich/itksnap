// Read the function library
include("Library");

// Open the test workspace
openWorkspace("tensor.itksnap");

// Enter snake mode
enterSnakeModeFullROI();

//=== Press the 3D view's Update button before the level set is initialized
//=== (there is nothing to mesh yet; this used to fail an assertion), then
//=== turn on continuous update, which stays on through the evolution below
engine.clickChild(mainwin, "btnUpdateMesh");
engine.sleep(1000);
engine.trigger("actionContinuous_Update");
engine.sleep(500);
engine.validateChildProperty(mainwin, "actionContinuous_Update", "checked", true);

//=== Entering classification mode
var snakepanel = engine.findChild(mainwin,"SnakeWizardPanel");
var combo = engine.findChild(snakepanel,"inPreprocessMode");
engine.comboBoxSelect(combo, "Classification");

//=== Show just the axial view
engine.clickChild(mainwin, "btnAxial");

// Get the axial panel
var panel0 = engine.findChild(mainwin,"panel0");
var sliceViewInternalWidget0 = engine.findChild(panel0, "sliceViewCanvas");

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
engine.clickChild(snakepanel, "btnClassifyTrain");

//=== Clear segmentation
engine.clickChild(snakepanel, "btnClassifyClearExamples");

//=== Go to bubble mode
engine.clickChild(snakepanel, "btnNextPreproc");
engine.sleep(1000);

//=== Go back to the first seed
setCursor(20,8,13);

//=== Validating speed image
engine.validateValue(readVoxelIntensity(4), 0.8, 0.4)

//=== Add a bubble
engine.clickChild(snakepanel, "btnAddBubble");

//=== Set bubble radius
engine.setChildProperty(snakepanel, "inBubbleRadius", "value", 4);

//=== Go to snake mode
engine.clickChild(snakepanel, "btnBubbleNext");
engine.sleep(1000);

//=== Set step size
engine.setChildProperty(snakepanel, "inStepSize", "value", 10);

//=== Run snake 100 iter
for(var i = 0; i < 10; i++)
{
  engine.clickChild(snakepanel, "btnSingleStep");

}

//=== Finish snake mode
engine.clickChild(snakepanel, "btnEvolutionNext");
engine.sleep(1000)

//=== Open volumes and statistics
engine.trigger("actionVolumesAndStatistics");

//=== Check the value
var dialog = engine.findChild(mainwin, "dlgStatistics");
var table =  engine.findChild(dialog, "tvVolumes");
var value =  engine.tableItemText(table,1,1);
engine.validateValue(value, 1200, 600);

