// Read the function library
include("Library");

// ITK-SNAP is started with --url and a workspace URL, as the URL handler on
// Windows and Linux starts it (see CMakeLists.txt). serve_slowly.py serves
// the files on 127.0.0.1:8920 over about 6 seconds each. The workspace used
// to be opened without a progress dialog, so nothing showed during the
// download and it could not be canceled.

//=== The progress dialog must show while the workspace downloads
var shown = false;
for (var i = 0; i < 25 && !shown; i++)
{
    var dlg = engine.findVisibleChild(mainwin, "dlgProgressReport");
    shown = (dlg != null);
    if (!shown)
        engine.sleep(200);
}
engine.print("Progress dialog shown during the download: " + shown);
engine.validateValue(shown, true);

//=== The workspace then opens, with its mesh layer
waitForMainImage(60);
engine.sleep(1000);
engine.trigger("actionLayerInspector");
engine.sleep(1000);
var inspector = engine.findChild(mainwin, "dlgLayerInspector");
var n = 0;
while (engine.findChild(inspector, "wgtRowDelegate_000" + n) != null)
    n++;
engine.close(inspector);
engine.print("Layers after opening the workspace URL: " + n);
engine.validateValue(n > 1, true);
