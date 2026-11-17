// Read the function library
include("Library");

// ITK-SNAP is started with a workspace given as a URL on the command line
// (see CMakeLists.txt). Wait until it is downloaded.
waitForMainImage();
engine.sleep(2000);

//=== Change the workspace: pick another color map for the main image
engine.trigger("actionLayerInspector");
engine.sleep(500);
var layerdialog = engine.findChild(mainwin, "dlgLayerInspector");
engine.setProperty(engine.findChild(layerdialog, "wgtRowDelegate_0000"), "selected", true);
var cmpcolormap = engine.findChild(layerdialog, "cmpColorMap");
engine.callChildMethod(layerdialog, "tabWidget", "setCurrentWidget", [cmpcolormap]);
engine.callMethod(engine.findChild(cmpcolormap, "inPreset"), "setCurrentIndex", [8]);
engine.sleep(500);
engine.invoke(layerdialog, "close");

//=== Unloading prompts to save the workspace
engine.trigger("actionUnload_All");
engine.sleep(1000);
var savedialog = engine.findWidget("dlgSaveModified");
if (!savedialog || !engine.getProperty(savedialog, "visible"))
    engine.testFailed("The unsaved changes dialog was not shown");
var layertable = engine.findChild(savedialog, "tableLayers");
engine.validateValue(engine.tableItemText(layertable, 0, 0), "Workspace");

// The workspace cannot be saved back to its URL, so it needs a file name and
// 'Save All' is not available
engine.validateChildProperty(savedialog, "btnSaveAll", "enabled", false);

//=== Saving it asks for a local file. This used to crash, because the file
//=== dialog was given no parent.
engine.clickChild(savedialog, "btnSave");
engine.sleep(2000);
var filedialog = engine.findWidget("dlgSimpleFile");
if (!filedialog || !engine.getProperty(filedialog, "visible"))
    engine.testFailed("Saving the workspace did not open the save dialog");
var suggested = String(engine.getChildProperty(filedialog, "inFilename", "text"));
engine.print("Suggested filename: " + suggested);
engine.validateValue(suggested.indexOf("http:") < 0, true);
engine.validateValue(suggested.endsWith("MRIcrop-with-mesh.itksnap"), true);

//=== Cancel both dialogs: the workspace stays loaded
engine.invoke(filedialog, "reject");
engine.sleep(1000);
engine.invoke(savedialog, "reject");
engine.sleep(1000);
waitForMainImage();
