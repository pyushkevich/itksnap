// Read the function library
include("Library");

// Navigating the snake wizard after training a classifier: going back from
// the evolution stage and cancelling the segmentation used to crash.

openMainImage("MRIcrop-orig.gipl.gz");
enterSnakeModeFullROI();

//=== Classification presegmentation
var snakepanel = engine.findChild(mainwin, "SnakeWizardPanel");
engine.comboBoxSelect(engine.findChild(snakepanel, "inPreprocessMode"), "Classification");

//=== Paint two examples with the paintbrush and train
var panel0 = engine.findChild(mainwin, "panel0");
var canvas = engine.findChild(panel0, "sliceViewCanvas");
engine.trigger("actionPaintbrush");
setForegroundLabel("Label 1");
setCursor(30, 40, 30);
engine.postKeyEvent(canvas, "Space");
setForegroundLabel("Label 2");
setCursor(50, 70, 30);
engine.postKeyEvent(canvas, "Space");
engine.clickChild(snakepanel, "btnClassifyTrain");
engine.sleep(1000);

//=== Go to the evolution stage with a bubble
engine.clickChild(snakepanel, "btnNextPreproc");
engine.sleep(1000);
engine.clickChild(snakepanel, "btnAddBubble");
engine.sleep(500);
engine.clickChild(snakepanel, "btnBubbleNext");
engine.sleep(1000);

//=== Go back twice, to the presegmentation stage (crashed: the level set
//=== layer had no image data when the scene extent was computed)
engine.clickChild(snakepanel, "btnEvolutionBack");
engine.sleep(1000);
engine.clickChild(snakepanel, "btnBubbleBack");
engine.sleep(1000);

//=== Cancel the segmentation (crashed: the layer inspector rows accessed the
//=== deleted snake mode layers)
engine.clickChild(snakepanel, "btnCancel");
engine.sleep(1500);

//=== Back in the main mode, the main image is still loaded
waitForMainImage();
engine.validateChildProperty(mainwin, "inCursorX", "maximum", 78);
