// Read the function library
include("Library");

// ITK-SNAP runs under remote_test_server.py (see CMakeLists.txt) with only a
// main image, given as an sftp:// URL. The segmentation is new and unnamed.
// The server sets HOME inside its root folder, next to the folder it serves.
waitForMainImage();
engine.sleep(2000);

var server_dir = engine.getEnv("HOME") + "/../sftp";

//=== Saving the new segmentation opens the wizard, suggesting a file next to
//=== the main image on the server
engine.trigger("actionSaveSegmentation");
engine.sleep(2000);
var dialog = engine.findChild(mainwin, "wizImageIO");
if (!dialog)
    engine.testFailed("Saving a new segmentation did not open the wizard");

var url = String(engine.getChildProperty(dialog, "inFilename", "text"));
engine.print("Suggested filename: " + url);
engine.validateValue(url.startsWith("sftp://testuser@127.0.0.1:"), true);
engine.validateValue(url.endsWith("/Untitled.nii.gz"), true);
engine.validateChildProperty(dialog, "btnBrowse", "enabled", false);
engine.validateChildProperty(dialog, "inFilename", "selectedText", "Untitled");

//=== Finish uploads it
engine.validateValue(engine.fileModifiedTime(server_dir + "/Untitled.nii.gz"), -1);
engine.clickChild(dialog, "qt_wizard_finish");
engine.sleep(3000);
engine.validateValue(engine.fileModifiedTime(server_dir + "/Untitled.nii.gz") > 0, true);
