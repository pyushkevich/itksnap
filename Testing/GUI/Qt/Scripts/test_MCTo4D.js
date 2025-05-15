// Read the function library
include("Library");

//=== Open the test workspace
openMainImage("tensor_rgb.nii.gz")

//=== Show the layer inspector
engine.trigger("actionLayerInspector");

//=== Select a specific overlay
let layerdialog = engine.findChild(mainwin,"dlgLayerInspector");
let rowdelegate = engine.findChild(layerdialog, "wgtRowDelegate_0000");
rowdelegate.setSelected(true);

//=== Trigger reload
engine.trigger("actionReloadAs4D", rowdelegate);

//=== Close the inspector dialog
engine.invoke(layerdialog, "close");

//=== Validation
setCursor4D(12, 16, 13, 1)
var intensity = readVoxelIntensity(0)
engine.validateValue(intensity, 103)


setCursor4D(20, 13, 16, 2)
var intensity = readVoxelIntensity(0)
engine.validateValue(intensity, 29)


setCursor4D(7, 22, 20, 3)
var intensity = readVoxelIntensity(0)
engine.validateValue(intensity, 105)
