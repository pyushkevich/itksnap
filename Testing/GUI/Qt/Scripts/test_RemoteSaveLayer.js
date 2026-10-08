// Read the function library
include("Library");

// ITK-SNAP is started with a main image and a segmentation given as URLs on
// the command line (see CMakeLists.txt). Wait until they are downloaded.
waitForMainImage();
engine.sleep(2000);

//=== The segmentation was loaded from a URL, so plain 'Save' cannot save it
//=== back there: it must open the Save As wizard, suggesting only the file name
engine.trigger("actionSaveSegmentation");
engine.sleep(2000);

var dialog = engine.findChild(mainwin, "wizImageIO");
if (!dialog)
    engine.testFailed("Saving a remote segmentation did not open the Save As wizard");

// The suggestion must be a plain local name: no trace of the URL, which could
// also show up in mangled form (e.g. ".../https:/raw.githubusercontent.com/...")
var suggested = String(engine.getChildProperty(dialog, "inFilename", "text"));
engine.print("Suggested filename: " + suggested);
engine.validateValue(suggested.indexOf("https:") < 0, true);
engine.validateValue(suggested.indexOf("githubusercontent") < 0, true);
engine.validateValue(suggested.endsWith("MRIcrop-seg.gipl.gz"), true);

//=== Save to a local file
var out_file = tempdir + "/RemoteSaveLayer_seg.nii.gz";
engine.setChildProperty(dialog, "inFilename", "text", out_file);
engine.sleep(500);
engine.clickChild(dialog, "qt_wizard_finish");
engine.sleep(2000);

//=== The layer now comes from the local file: the Layer Inspector shows the
//=== local filename and no longer shows the remote URL
engine.trigger("actionLayerInspector");
engine.sleep(500);
var dlg = engine.findChild(mainwin, "dlgLayerInspector");
var row = engine.findChild(dlg, "wgtRowDelegate_0001");
engine.setProperty(row, "selected", true);
engine.sleep(500);
engine.callChildMethod(dlg, "tabWidget", "setCurrentWidget", [engine.findChild(dlg, "cmpComponent")]);
engine.sleep(500);

var shown = String(engine.getChildProperty(dlg, "outFilename", "text"));
engine.print("Layer filename after save: " + shown);
engine.validateValue(shown.endsWith("RemoteSaveLayer_seg.nii.gz"), true);
engine.validateValue(shown.indexOf("://") < 0, true);
engine.validateChildProperty(dlg, "outRemoteURL", "text", "");
engine.invoke(dlg, "close");
