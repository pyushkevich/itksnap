// Read the function library
include("Library");

// Open the test image
openMainImage("MRIcrop-orig.gipl.gz");

// Make sure labels are ok
setForegroundLabel("Label 1");
setBackgroundLabel("All labels");

// Enter snake mode
enterSnakeMode(10, 30, 30, 32, 32, 32);

//=== Entering edge attraction mode
var snakepanel = engine.findChild(mainwin,"SnakeWizardPanel");
var combo = engine.findChild(snakepanel,"inPreprocessMode");
var index = engine.findItemRow(combo,"Edge Attraction");
combo.setCurrentIndex(index);

//=== Open edge parameters window
engine.findChild(snakepanel,"btnEdgeDetail").click();
var win_speed = engine.findChild(snakepanel, "dlgSpeedParameters");

//=== Set edge parameters
engine.findChild(win_speed,"inEdgeSmoothing").value = 0.4;
engine.findChild(win_speed,"inEdgeKappa").value = 0.03;
engine.findChild(win_speed,"inEdgeExponent").value = 3.5;

//=== Close the edge parameter window
engine.findChild(win_speed,"btnClose").click();

//=== Validating speed image
setCursor(19, 27, 17);
engine.validateFloatValue(readVoxelIntensity(1), 0.593, 0.001);

//=== Go to bubble mode
engine.findChild(snakepanel,"btnNextPreproc").click();
engine.sleep(1000);

//=== Add a bubble
setCursor(25, 9, 24);
engine.findChild(snakepanel,"inBubbleRadius").value=2;
engine.findChild(snakepanel,"btnAddBubble").click();

//=== Go to snake mode
engine.findChild(snakepanel,"btnBubbleNext").click();
engine.sleep(1000);

//=== Validating level set image
engine.validateFloatValue(readVoxelIntensity(2), -2.289, 0.1);

//=== Make sure there is no crash on reinitialization
engine.findChild(snakepanel,"btnEvolutionBack").click();
engine.sleep(1000);
engine.findChild(snakepanel,"btnBubbleNext").click();
engine.sleep(1000);

//=== Revalidating level set image
engine.validateFloatValue(readVoxelIntensity(2), -2.289, 0.1);

//=== Set step size
engine.findChild(snakepanel,"inStepSize").value = 10;

//=== Open evolution parameters dialog
engine.findChild(snakepanel,"btnEvolutionParameters").click();
var win_param = engine.findChild(snakepanel, "dlgSnakeParameters");

//=== Set forces to desired values
engine.findChild(win_param, "btnRestore").click();
engine.findChild(win_param, "btnClose").click();

//=== Run snake ten iter
for(i = 0; i < 10; i++)
  engine.findChild(snakepanel,"btnSingleStep").click();

//=== Validating level set image
setCursor(20, 9, 28);
engine.validateFloatValue(readVoxelIntensity(2), -0.4784, 0.1);

//=== Rewind and try again
engine.findChild(snakepanel,"btnRewind").click();

//=== Run snake ten iter again
for(i = 0; i < 10; i++)
  engine.findChild(snakepanel,"btnSingleStep").click();

//=== Validating level set image
engine.validateFloatValue(readVoxelIntensity(2), -0.4784, 0.1);

//=== Finish snake mode
engine.findChild(snakepanel,"btnEvolutionNext").click();

//=== Validate segmentation
value = engine.findChild(mainwin, "outLabelId").value;
engine.validateValue(value, 1);
