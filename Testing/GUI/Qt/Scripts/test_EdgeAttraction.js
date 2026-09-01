// Read the function library
include("Library");

// Open the test image
openMainImage("MRIcrop-orig.gipl.gz");

// Reset the labels
resetLabels();

// Make sure labels are ok
setForegroundLabel("Label 1");
setBackgroundLabel("All labels");

// Enter snake mode
enterSnakeMode(10, 30, 30, 32, 32, 32);

//=== Entering edge attraction mode
var snakepanel = engine.findChild(mainwin,"SnakeWizardPanel");
var combo = engine.findChild(snakepanel,"inPreprocessMode");
var index = engine.findItemRow(combo,"Edge Attraction");
engine.callMethod(combo, "setCurrentIndex", [index]);

//=== Open edge parameters window
engine.clickChild(snakepanel, "btnEdgeDetail");
var win_speed = engine.findChild(snakepanel, "dlgSpeedParameters");

//=== Set edge parameters
engine.setChildProperty(win_speed, "inEdgeSmoothing", "value", 0.4);
engine.setChildProperty(win_speed, "inEdgeKappa", "value", 0.03);
engine.setChildProperty(win_speed, "inEdgeExponent", "value", 3.5);

//=== Close the edge parameter window
engine.clickChild(win_speed, "btnClose");

//=== Validating speed image
setCursor(19, 27, 17);
engine.validateValue(readVoxelIntensity(1), 0.593, 0.001);

//=== Go to bubble mode
engine.clickChild(snakepanel, "btnNextPreproc");
engine.sleep(1000);

//=== Add a bubble
setCursor(25, 9, 24);
engine.setChildProperty(snakepanel, "inBubbleRadius", "value", 2.0);
engine.clickChild(snakepanel, "btnAddBubble");

//=== Go to snake mode
engine.clickChild(snakepanel, "btnBubbleNext");
engine.sleep(1000);

//=== Validating level set image
engine.validateValue(readVoxelIntensity(2), -2.289, 0.1);

//=== Make sure there is no crash on reinitialization
engine.clickChild(snakepanel, "btnEvolutionBack");
engine.sleep(1000);
engine.clickChild(snakepanel, "btnBubbleNext");
engine.sleep(1000);

//=== Revalidating level set image
engine.validateValue(readVoxelIntensity(2), -2.289, 0.1);

//=== Set step size
engine.setChildProperty(snakepanel, "inStepSize", "value", 10);

//=== Open evolution parameters dialog
engine.clickChild(snakepanel, "btnEvolutionParameters");
var win_param = engine.findChild(snakepanel, "dlgSnakeParameters");

//=== Set forces to desired values
engine.clickChild(win_param, "btnRestore");
engine.clickChild(win_param, "btnClose");

//=== Run snake ten iter
for(i = 0; i < 10; i++)
  engine.clickChild(snakepanel, "btnSingleStep");

//=== Validating level set image
setCursor(20, 9, 28);
engine.validateValue(readVoxelIntensity(2), -0.4784, 0.1);

//=== Rewind and try again
engine.clickChild(snakepanel, "btnRewind");

//=== Run snake ten iter again
for(i = 0; i < 10; i++)
  engine.clickChild(snakepanel, "btnSingleStep");

//=== Validating level set image
engine.validateValue(readVoxelIntensity(2), -0.4784, 0.1);

//=== Finish snake mode
engine.clickChild(snakepanel, "btnEvolutionNext");

//=== Validate segmentation
value = engine.getChildProperty(mainwin, "outLabelId", "value");
engine.validateValue(value, 1);
