// Read the function library
include("Library");

// Load the image with NaNs
openMainImage("nan.mha")

//=== Go to a NaN pixel and check its intensity
setCursor(35, 58, 33)
var intensity = readVoxelIntensity(0)
engine.validateValue(intensity, "nan")

//=== Show the layer inspector contrast dialog
engine.trigger("actionImage_Contrast");

//=== Auto-adjust contrast
var layerdialog = engine.findChild(mainwin,"dlgLayerInspector");
var panel = engine.findChild(layerdialog, "grpWindow");
engine.findChild(panel, "btnAuto").click();

//=== Read values from the contrast panel
var range_min = engine.findChild(panel, "inMin").value
var range_max = engine.findChild(panel, "inMax").value
engine.validateFloatValue(range_min, 0, 20);
engine.validateFloatValue(range_max, 115, 20);
