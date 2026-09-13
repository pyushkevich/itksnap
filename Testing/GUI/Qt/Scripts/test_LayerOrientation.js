// Read the function library
include("Library");

// Open a workspace whose layers have different orientations: the main image
// t1_chunk is RPI, the overlays t2_chunk and multi_chunk are oblique
openWorkspace("diffspace.itksnap");

// Read the orientation shown on the Info tab of the Layer Inspector for a row
function getLayerOrientation(rowObjectName)
{
    var dlg = engine.findChild(mainwin, "dlgLayerInspector");
    var row = engine.findChild(dlg, rowObjectName);
    engine.setProperty(row, "selected", true);
    engine.sleep(500);

    var text = engine.getChildProperty(dlg, "outRAI", "text");
    engine.print("Orientation of " + rowObjectName + ": " + text);
    return text;
}

//=== Open the Layer Inspector on the Info tab
engine.trigger("actionLayerInspector");
engine.sleep(500);
var dlg = engine.findChild(mainwin, "dlgLayerInspector");
engine.callChildMethod(dlg, "tabWidget", "setCurrentWidget", [engine.findChild(dlg, "cmpInfo")]);
engine.sleep(500);

//=== Each layer should report its own orientation (#185)
engine.validateValue(getLayerOrientation("wgtRowDelegate_0000"), "RPI");

var t2 = getLayerOrientation("wgtRowDelegate_0001");
if (t2.indexOf("Oblique") != 0)
    engine.testFailed("Overlay t2_chunk should be oblique, but shows " + t2);

//=== Switching back to the main image should restore its orientation
engine.validateValue(getLayerOrientation("wgtRowDelegate_0000"), "RPI");

engine.invoke(dlg, "close");
