// Read the function library
include("Library");

// ITK-SNAP runs under remote_test_server.py (see CMakeLists.txt) with a
// workspace from a mock Flywheel analysis made by a gear, which takes no
// uploads. Files from it are saved to the analysis's session instead. The
// server sets HOME inside its root folder, next to the folders it serves.
waitForMainImage();
engine.sleep(2000);

var session_dir = engine.getEnv("HOME") + "/../fw/sessions/e0001";

// Whether a file dialog is showing
function fileDialogShown()
{
    var dlg = engine.findWidget("dlgSimpleFile");
    return dlg != null && engine.getProperty(dlg, "visible");
}

//=== The new segmentation is suggested in the analysis's session
engine.trigger("actionSaveSegmentation");
engine.sleep(2000);
var dialog = engine.findChild(mainwin, "wizImageIO");
if (!dialog)
    engine.testFailed("Saving a new segmentation did not open the wizard");
var url = String(engine.getChildProperty(dialog, "inFilename", "text"));
engine.print("Suggested segmentation: " + url);
engine.validateValue(url.startsWith("fw://127.0.0.1:"), true);
engine.validateValue(url.endsWith("/sessions/e0001/files/Untitled.nii.gz"), true);
engine.clickChild(dialog, "qt_wizard_finish");
engine.sleep(3000);
var t_seg = engine.fileModifiedTime(session_dir + "/Untitled.nii.gz");
engine.validateValue(t_seg > 0, true);

//=== Saving it again goes straight to the session
engine.sleep(1100);
engine.trigger("actionSaveSegmentation");
engine.sleep(3000);
if (engine.findChild(mainwin, "wizImageIO"))
    engine.testFailed("Saving a segmentation in a session opened the wizard");
engine.validateValue(engine.fileModifiedTime(session_dir + "/Untitled.nii.gz") > t_seg, true);

//=== The workspace cannot be saved back to the analysis: Save Workspace asks,
//=== suggesting the same name in the session
engine.trigger("actionSaveWorkspace");
engine.sleep(2000);
if (!fileDialogShown())
    engine.testFailed("Saving a workspace from an analysis did not open the file dialog");
var filedialog = engine.findWidget("dlgSimpleFile");
var ws_url = String(engine.getChildProperty(filedialog, "inFilename", "text"));
engine.print("Suggested workspace: " + ws_url);
engine.validateValue(ws_url.endsWith("/sessions/e0001/files/MRIcrop-with-mesh.itksnap"), true);
engine.invoke(filedialog, "accept");
engine.sleep(3000);
engine.validateValue(engine.fileModifiedTime(session_dir + "/MRIcrop-with-mesh.itksnap") > 0, true);
