// Read the function library
include("Library");

// Open the test workspace
openWorkspace("tensor.itksnap");

//=== Show the layer inspector
engine.findChild(mainwin,"actionLayerInspector").trigger();

//=== Select a specific overlay
var layerdialog = engine.findChild(mainwin,"dlgLayerInspector");
var rowdelegate = engine.findChild(layerdialog, "wgtRowDelegate_0003");
rowdelegate.setSelected(true);

//=== Toggle the layout
engine.findChild(layerdialog, "actionLayoutToggle").trigger();

//=== Go to the color map widget
var cmpcolormap = engine.findChild(layerdialog, "cmpColorMap");
engine.findChild(layerdialog, "tabWidget").setCurrentWidget(cmpcolormap);

//=== Select the color map preset we want
var inpreset = engine.findChild(cmpcolormap, "inPreset");
inpreset.setCurrentIndex(8);

//=== Close the inspector dialog
engine.invoke(layerdialog, "close");

//=== Trigger the unload all action
engine.findChild(mainwin,"actionUnload_All").trigger();

//=== Check that the unsaved changes dialog is open
var savedialog = engine.findWidget("dlgSaveModified");
if(!savedialog.visible)
    engine.testFailed("Saved dialog was not shown")

//=== Count the number of entries in the dialog
var layertable = engine.findChild(savedialog, "tableLayers");
var entry = engine.tableItemText(layertable, 0, 0);
engine.validateValue(entry, "Workspace");
