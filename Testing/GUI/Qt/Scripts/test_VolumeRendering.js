// Read the function library
include("Library");

//=== Open the test workspace
openWorkspace("img4d_11f_volren.itksnap");

//=== Show the layer inspector
engine.trigger("actionLayerInspector");

//=== Select a specific overlay
let layerdialog = engine.findChild(mainwin,"dlgLayerInspector");
let rowdelegate = engine.findChild(layerdialog, "wgtRowDelegate_0000");
engine.setProperty(rowdelegate, "selected", true);

//=== Enable volume rendering
engine.trigger("actionVolumeEnable", rowdelegate);

//=== Close the inspector dialog
engine.invoke(layerdialog, "close");

// Select the 4D Property Group
var grp4D = engine.findChild(mainwin, "grp4DProperties");

var btnReplay = engine.findChild(grp4D, "btn4DReplay");
engine.click(btnReplay);
engine.sleep(2000);
engine.click(btnReplay);

engine.setChildProperty(grp4D, "in4DReplayInterval", "text", "200");
engine.click(btnReplay);
engine.sleep(3000);
engine.click(btnReplay);

//=== Disable volume rendering
engine.trigger("actionVolumeEnable", rowdelegate);
engine.sleep(500);
