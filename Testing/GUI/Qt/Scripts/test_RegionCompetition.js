// Read the function library
include("Library");

// Open the test image
openMainImage("MRIcrop-orig.gipl.gz");

// Make sure labels are ok
setForegroundLabel("Label 1");
setBackgroundLabel("All labels");

// Enter snake mode
enterSnakeMode(10, 10, 10, 32, 32, 32);

//=== Entering thresholding mode
var snakepanel = engine.findChild(mainwin,"SnakeWizardPanel");
var combo = engine.findChild(snakepanel,"inPreprocessMode");
var index = engine.findItemRow(combo,"Thresholding");
combo.setCurrentIndex(index);

//=== Setting thresholds
engine.findChild(snakepanel,"inThreshLowerSpin").value = 24.0;
engine.findChild(snakepanel,"inThreshUpperSpin").value = 57.0;

//=== Validating speed image
setCursor(17, 15, 20);
engine.validateFloatValue(readVoxelIntensity(1), -0.2263, 0.0001)

//=== Go to bubble mode
engine.findChild(snakepanel,"btnNextPreproc").click();
engine.sleep(1000);

//=== Add a bubble
engine.findChild(snakepanel,"btnAddBubble").click();

//=== Go to snake mode
engine.findChild(snakepanel,"btnBubbleNext").click();
engine.sleep(1000);

//=== Validating level set image
engine.validateValue(readVoxelIntensity(2), -4);

//=== Set step size
engine.findChild(snakepanel,"inStepSize").value = 10;

//=== Open evolution parameters dialog
engine.findChild(snakepanel,"btnEvolutionParameters").click();
var win_param = engine.findChild(snakepanel, "dlgSnakeParameters");

//=== Set forces to desired values
engine.findChild(win_param, "inZhuAlphaSimple").value = 1.0;
engine.findChild(win_param, "inZhuBetaSimple").value = 0.2;
engine.findChild(win_param, "btnClose").click();

//=== Run snake one iter
engine.findChild(snakepanel,"btnSingleStep").click();

//=== Run snake one iter
engine.findChild(snakepanel,"btnSingleStep").click();

//=== Validating level set image
setCursor(16, 15, 20);
engine.validateFloatValue(readVoxelIntensity(2), -0.9371, 0.2)

//=== Finish snake mode
engine.findChild(snakepanel,"btnEvolutionNext").click()

//=== Validate segmentation
value = engine.findChild(mainwin, "outLabelId").value
engine.validateValue(value, 1)
