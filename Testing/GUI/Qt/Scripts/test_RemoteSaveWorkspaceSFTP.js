// Read the function library
include("Library");

// ITK-SNAP runs under remote_test_server.py (see CMakeLists.txt) with a
// workspace given as an sftp:// URL. The server sets HOME inside its root
// folder, next to the folder it serves.
waitForMainImage();
engine.sleep(2000);

var server_dir = engine.getEnv("HOME") + "/../sftp";
var server_ws = server_dir + "/MRIcrop-with-mesh.itksnap";
var server_copy = server_dir + "/RemoteWorkspaceGUI.itksnap";
var t_before = engine.fileModifiedTime(server_ws);
engine.validateValue(t_before > 0, true);

// Whether a file dialog is showing
function fileDialogShown()
{
    var dlg = engine.findWidget("dlgSimpleFile");
    return dlg != null && engine.getProperty(dlg, "visible");
}

//=== Save Workspace uploads it back to its URL, without a dialog
engine.trigger("actionSaveWorkspace");
engine.sleep(3000);
if (fileDialogShown())
    engine.testFailed("Saving a workspace from an sftp:// URL opened a file dialog");
var t_after = engine.fileModifiedTime(server_ws);
engine.print("Workspace modified at " + t_before + ", after Save at " + t_after);
engine.validateValue(t_after > t_before, true);

//=== Save Workspace As shows the URL, with Browse disabled
engine.trigger("actionSaveWorkspaceAs");
engine.sleep(2000);
var dialog = engine.findWidget("dlgSimpleFile");
if (!fileDialogShown())
    engine.testFailed("Save Workspace As did not open the file dialog");
var url = String(engine.getChildProperty(dialog, "inFilename", "text"));
engine.print("Suggested filename: " + url);
engine.validateValue(url.startsWith("sftp://testuser@127.0.0.1:"), true);
engine.validateValue(url.endsWith("/MRIcrop-with-mesh.itksnap"), true);
engine.validateChildProperty(dialog, "btnBrowse", "enabled", false);

//=== Save under another name on the server
var new_url = url.substring(0, url.lastIndexOf("/") + 1) + "RemoteWorkspaceGUI.itksnap";
engine.setChildProperty(dialog, "inFilename", "text", new_url);
engine.sleep(500);
engine.invoke(dialog, "accept");
engine.sleep(3000);
var t_copy = engine.fileModifiedTime(server_copy);
engine.validateValue(t_copy > 0, true);

//=== A local overlay: saving the workspace to the server warns that others
//=== cannot open it. Cancel leaves the server's copy as it was.
openOverlay("MRIcrop-seg.gipl.gz");
engine.sleep(1100);
engine.trigger("actionSaveWorkspace");
engine.sleep(1500);
var mbox = engine.findWidget("mboxLocalFilesInRemoteWorkspace");
if (!mbox || !engine.getProperty(mbox, "visible"))
    engine.testFailed("No warning about a local file in a workspace saved to a server");
engine.clickChild(mbox, "btnCancel");
engine.sleep(1500);
engine.validateValue(engine.fileModifiedTime(server_copy), t_copy);

//=== With unsaved changes, unloading offers to save the workspace, which
//=== goes back to the server without asking for a file (after the warning)
engine.trigger("actionUnload_All");
engine.sleep(1000);
var savedialog = engine.findWidget("dlgSaveModified");
if (!savedialog || !engine.getProperty(savedialog, "visible"))
    engine.testFailed("The unsaved changes dialog was not shown");
var layertable = engine.findChild(savedialog, "tableLayers");
engine.validateValue(engine.tableItemText(layertable, 0, 0), "Workspace");
engine.validateValue(String(engine.tableItemText(layertable, 0, 1)), new_url);
engine.validateChildProperty(savedialog, "btnSaveAll", "enabled", true);

engine.clickChild(savedialog, "btnSave");
engine.sleep(1500);
if (fileDialogShown())
    engine.testFailed("Saving the workspace from the unsaved changes dialog opened a file dialog");
mbox = engine.findWidget("mboxLocalFilesInRemoteWorkspace");
if (!mbox || !engine.getProperty(mbox, "visible"))
    engine.testFailed("No warning about a local file from the unsaved changes dialog");
engine.clickChild(mbox, "btnSave");
engine.sleep(3000);
var t_copy2 = engine.fileModifiedTime(server_copy);
engine.print("Copy modified at " + t_copy + ", after the prompt at " + t_copy2);
engine.validateValue(t_copy2 > t_copy, true);
