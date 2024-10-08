// Read the function library
include("Library");

// Load the image with NaNs
openMainImage("warp_image.nii.gz")

//=== Show the layer inspector
engine.trigger("actionLayerInspector");

//=== Select a specific overlay
let layerdialog = engine.findChild(mainwin,"dlgLayerInspector");
let rowdelegate = engine.findChild(layerdialog, "wgtRowDelegate_0000");
rowdelegate.setSelected(true);
engine.trigger("Grid", rowdelegate);

engine.sleep(100);
