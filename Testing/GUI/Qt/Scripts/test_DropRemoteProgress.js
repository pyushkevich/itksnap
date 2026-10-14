// Read the function library
include("Library");

// A URL opened while an image is loaded brings up the drop dialog. Choosing
// "load as additional image" there must show the download's progress while
// it runs. The progress widget used to wait for the dialog (which is modal)
// to close, so it showed only after the download, at 100%.
//
// The test is run by serve_slowly.py (see CMakeLists.txt), which serves the
// test data on 127.0.0.1:8917, sending each file over about 6 seconds.
openMainImage("MRIcrop-orig.gipl.gz");

// Number of layers in the Layer Inspector
function countLayers()
{
    engine.trigger("actionLayerInspector");
    engine.sleep(1000);
    var inspector = engine.findChild(mainwin, "dlgLayerInspector");
    var n = 0;
    while (engine.findChild(inspector, "wgtRowDelegate_000" + n) != null)
        n++;
    engine.close(inspector);
    engine.sleep(500);
    return n;
}
var nLayers = countLayers();

//=== Open a URL, as the URL handler or a second ITK-SNAP would
engine.callMethod(mainwin, "LoadDroppedFile",
                  ["http://127.0.0.1:8917/MRIcrop-orig.gipl.gz", false]);
engine.sleep(1000);

var dropDialog = engine.findVisibleChild(mainwin, "DropActionDialog");
if (!dropDialog)
    engine.testFailed("Opening a URL did not show the drop dialog");

//=== Load it as an additional image
engine.clickChild(dropDialog, "btnLoadOverlay");

//=== The progress widget must show within 3 seconds, long before the
//=== download is done
var shown = false;
for (var i = 0; i < 15 && !shown; i++)
{
    engine.sleep(200);
    shown = engine.getChildProperty(mainwin, "progressReport", "visible");
}
engine.print("Progress shown during the download: " + shown);
engine.validateValue(shown, true);
engine.validateValue(engine.getProperty(dropDialog, "visible"), false);

//=== Wait for the download to finish and the progress widget to go away
var done = false;
for (var i = 0; i < 60 && !done; i++)
{
    engine.sleep(500);
    done = !engine.getChildProperty(mainwin, "progressReport", "visible");
}
engine.validateValue(done, true);
engine.sleep(1000);

//=== The image was loaded as an additional image (an overlay)
engine.validateValue(countLayers(), nLayers + 1);
