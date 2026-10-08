// Read the function library
include("Library");

// ITK-SNAP is started with a workspace given as a URL on the command line
// (see CMakeLists.txt). Wait until it is downloaded.
waitForMainImage();
engine.sleep(2000);

//=== The workspace was opened from a URL, so plain 'Save Workspace' cannot
//=== save it back there: it must ask for a file, suggesting only the name
engine.trigger("actionSaveWorkspace");
engine.sleep(2000);

var dialog = engine.findChild(mainwin, "dlgSimpleFile");
if (!dialog)
    engine.testFailed("Saving a remote workspace did not open the save dialog");

// The suggestion must be a plain local name: no trace of the URL, which could
// also show up in mangled form (e.g. ".../https:/raw.githubusercontent.com/...")
var suggested = String(engine.getChildProperty(dialog, "inFilename", "text"));
engine.print("Suggested filename: " + suggested);
engine.validateValue(suggested.indexOf("https:") < 0, true);
engine.validateValue(suggested.indexOf("githubusercontent") < 0, true);
engine.validateValue(suggested.endsWith("MRIcrop-with-mesh.itksnap"), true);

//=== Save to a local file
var out_file = tempdir + "/RemoteSaveWorkspace.itksnap";
engine.setChildProperty(dialog, "inFilename", "text", out_file);
engine.invoke(dialog, "accept");
engine.sleep(2000);

//=== The workspace is now the local file
engine.trigger("actionSaveWorkspace");
engine.sleep(1000);
var dialog2 = engine.findChild(mainwin, "dlgSimpleFile");
engine.validateValue(dialog2 == null || !engine.getProperty(dialog2, "visible"), true);

//=== Reopen the saved local workspace: its main layer must still be loaded
//=== from the remote URL (not from a mangled local path or a temp file)
openWorkspace_abs(out_file);
waitForMainImage();

engine.trigger("actionLayerInspector");
engine.sleep(500);
var dlg = engine.findChild(mainwin, "dlgLayerInspector");
var row = engine.findChild(dlg, "wgtRowDelegate_0000");
engine.setProperty(row, "selected", true);
engine.sleep(500);
engine.callChildMethod(dlg, "tabWidget", "setCurrentWidget", [engine.findChild(dlg, "cmpComponent")]);
engine.sleep(500);
// For a remote layer the inspector shows the file name and, separately, the URL
var url = String(engine.getChildProperty(dlg, "outRemoteURL", "text"));
engine.print("Main layer URL after reopening: " + url);
engine.validateValue(url.startsWith("https://"), true);
engine.validateValue(url.endsWith("MRIcrop-orig.gipl.gz"), true);
engine.invoke(dlg, "close");

function openWorkspace_abs(file)
{
    engine.trigger("actionOpenWorkspace");
    engine.sleep(2000);
    var dlg = engine.findChild(mainwin, "dlgSimpleFile");
    engine.setChildProperty(dlg, "inFilename", "text", file);
    engine.invoke(dlg, "accept");
    engine.sleep(4000);
}
