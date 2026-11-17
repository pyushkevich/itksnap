// Read the function library
include("Library");

// ITK-SNAP runs under remote_test_server.py (see CMakeLists.txt), with a main
// image and a segmentation given as sftp:// URLs on the command line. The
// server sets HOME inside its root folder, next to the folder it serves.
waitForMainImage();
engine.sleep(2000);

var server_dir = engine.getEnv("HOME") + "/../sftp";
var server_seg = server_dir + "/MRIcrop-seg.gipl.gz";
var t_before = engine.fileModifiedTime(server_seg);
engine.print("Server segmentation modified at " + t_before);
engine.validateValue(t_before > 0, true);

//=== Plain 'Save' uploads the segmentation back to its URL, without a wizard
engine.trigger("actionSaveSegmentation");
engine.sleep(3000);
if (engine.findChild(mainwin, "wizImageIO"))
    engine.testFailed("Saving a segmentation from an sftp:// URL opened the wizard");
var t_after = engine.fileModifiedTime(server_seg);
engine.print("Server segmentation modified at " + t_after + " after Save");
engine.validateValue(t_after > t_before, true);

//=== 'Save As' shows the full URL, and Browse is disabled
engine.trigger("actionSaveSegmentationAs");
engine.sleep(2000);
var dialog = engine.findChild(mainwin, "wizImageIO");
if (!dialog)
    engine.testFailed("Save As did not open the wizard");

var url = String(engine.getChildProperty(dialog, "inFilename", "text"));
engine.print("Suggested filename: " + url);
engine.validateValue(url.startsWith("sftp://testuser@127.0.0.1:"), true);
engine.validateValue(url.endsWith("/MRIcrop-seg.gipl.gz"), true);
engine.validateChildProperty(dialog, "btnBrowse", "enabled", false);

//=== An http URL cannot be saved to: an error is shown and Finish is disabled
engine.setChildProperty(dialog, "inFilename", "text", "http://127.0.0.1:8980/seg.nii.gz");
engine.sleep(500);
var error = String(engine.getChildProperty(dialog, "outError", "text"));
engine.print("Error for an http URL: " + error);
engine.validateValue(error.indexOf("cannot be saved") >= 0, true);
engine.validateChildProperty(dialog, "qt_wizard_finish", "enabled", false);
engine.validateChildProperty(dialog, "btnBrowse", "enabled", false);

//=== A local filename enables Browse again
engine.setChildProperty(dialog, "inFilename", "text", tempdir + "/RemoteSaveSFTP_seg.nii.gz");
engine.sleep(500);
engine.validateChildProperty(dialog, "btnBrowse", "enabled", true);
engine.validateChildProperty(dialog, "qt_wizard_finish", "enabled", true);

//=== Save under a new name on the server
var new_url = url.substring(0, url.lastIndexOf("/") + 1) + "MRIcrop-seg-edited.nii.gz";
engine.setChildProperty(dialog, "inFilename", "text", new_url);
engine.sleep(500);
engine.validateChildProperty(dialog, "btnBrowse", "enabled", false);
engine.validateChildProperty(dialog, "outError", "text", "");
engine.clickChild(dialog, "qt_wizard_finish");
engine.sleep(3000);
engine.validateValue(engine.fileModifiedTime(server_dir + "/MRIcrop-seg-edited.nii.gz") > 0, true);

//=== The Layer Inspector shows the new URL
engine.trigger("actionLayerInspector");
engine.sleep(500);
var dlg = engine.findChild(mainwin, "dlgLayerInspector");
var row = engine.findChild(dlg, "wgtRowDelegate_0001");
engine.setProperty(row, "selected", true);
engine.sleep(500);
engine.callChildMethod(dlg, "tabWidget", "setCurrentWidget", [engine.findChild(dlg, "cmpComponent")]);
engine.sleep(500);

var shown = String(engine.getChildProperty(dlg, "outFilename", "text"));
var shown_url = String(engine.getChildProperty(dlg, "outRemoteURL", "text"));
engine.print("Layer filename after Save As: " + shown);
engine.print("Layer URL after Save As: " + shown_url);
engine.validateValue(shown.endsWith("MRIcrop-seg-edited.nii.gz"), true);
engine.validateValue(shown_url, new_url);
engine.invoke(dlg, "close");
