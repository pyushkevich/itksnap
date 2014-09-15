// Read the function library
thread.source("Library");

// Open the test image
openMainImage("MRIcrop-orig.gipl.gz");

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
thread.wait(1000);

//=== Add a bubble
engine.findChild(snakepanel,"btnAddBubble").click();

//=== Go to snake mode
engine.findChild(snakepanel,"btnBubbleNext").click();
thread.wait(1000);

//=== Validating level set image
engine.validateValue(readVoxelIntensity(2), -4);

//=== Set step size
engine.findChild(snakepanel,"inStepSize").value = 10;

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
