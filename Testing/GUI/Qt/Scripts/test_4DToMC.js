// Read the function library
include("Library");

//=== Open the test workspace
openWorkspace("img4d_11f_volren.itksnap");

//=== Show the layer inspector
engine.findChild(mainwin,"actionLayerInspector").trigger();

//=== Select a specific overlay
let layerdialog = engine.findChild(mainwin,"dlgLayerInspector");
let rowdelegate = engine.findChild(layerdialog, "wgtRowDelegate_0000");
rowdelegate.setSelected(true);

//=== Enable volume rendering
let actionReload = engine.findChild(rowdelegate, "actionReloadAsMultiComponent");

actionReload.trigger();

//=== Close the inspector dialog
engine.invoke(layerdialog, "close");
engine.sleep(500)

//=== Switch multicomponent display
rowdelegate = engine.findChild(layerdialog, "wgtRowDelegate_0000");
let mode = engine.findChild(rowdelegate, "Mag");
mode.trigger();

setCursor(15, 19, 20)
var intensity = readVoxelIntensity(0)
engine.validateValue(intensity, 249.5)

engine.sleep(100);

mode = engine.findChild(rowdelegate, "Avg");
mode.trigger();

setCursor(21, 18, 20)
var intensity = readVoxelIntensity(0)
engine.validateValue(intensity, 64.25)

engine.sleep(100);

mode = engine.findChild(rowdelegate, "Max");
mode.trigger();

setCursor(30, 19, 25)
var intensity = readVoxelIntensity(0)
engine.validateValue(intensity, 52.77)

engine.sleep(500);
