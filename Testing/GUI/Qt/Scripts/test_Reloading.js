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

//=== trigger reload
let actionReload = engine.findChild(rowdelegate, "actionReloadFromFile");
actionReload.trigger();

//=== Close the inspector dialog
engine.invoke(layerdialog, "close");

// Select the 4D Property Group
var grp4D = engine.findChild(mainwin, "grp4DProperties");

var btnReplay = engine.findChild(grp4D, "btn4DReplay");
btnReplay.click();
engine.sleep(1000);
btnReplay.click();
