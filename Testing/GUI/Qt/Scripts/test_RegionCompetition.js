//--- source OpenImage

// Find some of the frequently used objects
var voxtable = engine.findChild(mainwin, "tableVoxelUnderCursor")

//--- sleep 600
engine.print("Entering snake mode")
var action = engine.findChild(mainwin,"actionSnake");
action.trigger();

//--- sleep 600
var roipanel = engine.findChild(mainwin, "pageSnakeTool")
engine.print("Setting up ROI")
engine.findChild(roipanel,"inIndexX").value = 10
engine.findChild(roipanel,"inIndexY").value = 10
engine.findChild(roipanel,"inIndexZ").value = 10
engine.findChild(roipanel,"inSizeX").value = 32
engine.findChild(roipanel,"inSizeY").value = 32
engine.findChild(roipanel,"inSizeZ").value = 32

//--- sleep 600
engine.print("Pushing the Segment3D button")
engine.findChild(roipanel,"btnAuto").click()

//--- sleep 2000
engine.print("Entering thresholding mode")
var snakepanel = engine.findChild(mainwin,"SnakeWizardPanel")
var combo = engine.findChild(snakepanel,"inPreprocessMode")
var index = engine.findItemRow(combo,"Thresholding")
combo.setCurrentIndex(index)

//--- sleep 600
engine.print("Setting thresholds")
engine.findChild(snakepanel,"inThreshLowerSlider").value = 24.0
engine.findChild(snakepanel,"inThreshLowerSlider").value = 57.0

//--- sleep 600
engine.print("Setting cursor position")
engine.findChild(mainwin, "inCursorX").value = 17
engine.findChild(mainwin, "inCursorY").value = 15
engine.findChild(mainwin, "inCursorZ").value = 20

//--- sleep 600
engine.print("Validating speed image")
value = engine.tableItemText(voxtable, 1, 1)
engine.validateFloatValue(value, -0.2263, 0.0001)

//--- sleep 600
engine.print("Go to bubble mode")
engine.findChild(snakepanel,"btnNextPreproc").click()

//--- sleep 2000
engine.print("Add a bubble")
engine.findChild(snakepanel,"btnAddBubble").click()

//--- sleep 600
engine.print("Go to snake mode")
engine.findChild(snakepanel,"btnBubbleNext").click()

//--- sleep 2000
engine.print("Validating level set image")
value = engine.tableItemText(voxtable, 2, 1)
engine.validateValue(value, -4)

//--- sleep 600
engine.print("Set step size")
engine.findChild(snakepanel,"inStepSize").value = 10

//--- sleep 600
engine.print("Run snake one iter")
engine.findChild(snakepanel,"btnSingleStep").click()

//--- sleep 2000
engine.print("Run snake one iter")
engine.findChild(snakepanel,"btnSingleStep").click()

//--- sleep 2000
engine.print("Setting cursor position")
engine.findChild(mainwin, "inCursorX").value = 16
engine.findChild(mainwin, "inCursorY").value = 15
engine.findChild(mainwin, "inCursorZ").value = 20

//--- sleep 600
engine.print("Validating level set image")
value = engine.tableItemText(voxtable, 2, 1)
engine.validateFloatValue(value, -0.9371, 0.0001)

//--- sleep 600
engine.print("Finish snake mode")
engine.findChild(snakepanel,"btnEvolutionNext").click()

//--- sleep 2000
engine.print("Validate segmentation")
value = engine.findChild(mainwin, "outLabelId").value
engine.validateValue(value, 1)
