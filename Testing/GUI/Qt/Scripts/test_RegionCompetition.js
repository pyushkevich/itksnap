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
enterSnakeMode(10, 10, 10, 32, 32, 32);

//=== Entering thresholding mode
var snakepanel = engine.findChild(mainwin,"SnakeWizardPanel");
var combo = engine.findChild(snakepanel,"inPreprocessMode");
var index = engine.findItemRow(combo,"Thresholding");
engine.callMethod(combo, "setCurrentIndex", [index]);

//=== Setting thresholds
engine.setChildProperty(snakepanel, "inThreshLowerSpin", "value", 24.0);
engine.setChildProperty(snakepanel, "inThreshUpperSpin", "value", 57.0);

//=== Validating speed image
setCursor(17, 15, 20);
engine.validateValue(readVoxelIntensity(1), -0.2263, 0.0001)

//=== Go to bubble mode
engine.clickChild(snakepanel, "btnNextPreproc");
engine.sleep(1000);

//=== Add a bubble
engine.clickChild(snakepanel, "btnAddBubble");

//=== Go to snake mode
engine.clickChild(snakepanel, "btnBubbleNext");
engine.sleep(1000);

//=== Validating level set image
engine.validateValue(readVoxelIntensity(2), -4);

//=== Set step size
engine.setChildProperty(snakepanel, "inStepSize", "value", 10);

//=== Open evolution parameters dialog
engine.clickChild(snakepanel, "btnEvolutionParameters");
var win_param = engine.findChild(snakepanel, "dlgSnakeParameters");

//=== Set forces to desired values
engine.setChildProperty(win_param, "inZhuAlphaSimple", "value", 1.0);
engine.setChildProperty(win_param, "inZhuBetaSimple", "value", 0.2);
engine.clickChild(win_param, "btnClose");

//=== Run snake one iter
engine.clickChild(snakepanel, "btnSingleStep");

//=== Run snake one iter
engine.clickChild(snakepanel, "btnSingleStep");

//=== Validating level set image
setCursor(16, 15, 20);
engine.validateValue(readVoxelIntensity(2), -0.9371, 0.2)

//=== Finish snake mode
engine.clickChild(snakepanel, "btnEvolutionNext");

//=== Validate segmentation
value = engine.getChildProperty(mainwin, "outLabelId", "value");
engine.validateValue(value, 1)
