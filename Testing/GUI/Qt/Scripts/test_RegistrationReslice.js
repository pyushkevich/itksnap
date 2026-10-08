// Read the function library
include("Library");

// Reslicing the moving image in the registration dialog must produce an image
// on the main image's grid (the registration is relative to the main image),
// even when the active segmentation, which defines the reference space, has
// a different grid.

openMainImage("MRIcrop-orig.gipl.gz");

//=== Make a segmentation on a 0.4mm grid the active one (the reference space)
openSegmentation("MRIcrop-seg-hippoR-04mm.nii.gz");

//=== Load the same image again as an overlay, to serve as the moving image.
//=== (This is done after the segmentation: the overlay wizard is kept around
//=== under the same object name as the other image wizards.)
engine.trigger("actionAdd_Overlay");
engine.sleep(2000);
var wiz = engine.findChild(mainwin, "wizImageIO");
engine.setChildProperty(wiz, "inFilename", "text", datadir + "/MRIcrop-orig.gipl.gz");
engine.clickChild(wiz, "qt_wizard_commit");
engine.sleep(1000);
engine.clickChild(wiz, "qt_wizard_finish");
engine.sleep(1000);

// Spacing and origin of a layer, from the Layer Inspector's Info tab
function layerGeometry(rowObjectName)
{
    engine.trigger("actionLayerInspector");
    engine.sleep(500);
    var dlg = engine.findChild(mainwin, "dlgLayerInspector");
    var row = engine.findChild(dlg, rowObjectName);
    engine.setProperty(row, "selected", true);
    engine.sleep(500);
    engine.callChildMethod(dlg, "tabWidget", "setCurrentWidget", [engine.findChild(dlg, "cmpInfo")]);
    engine.sleep(500);
    var geom = [];
    var fields = ["outSpacingX", "outSpacingY", "outSpacingZ", "outOriginX", "outOriginY", "outOriginZ"];
    for (var i = 0; i < fields.length; i++)
        geom.push(parseFloat(engine.getChildProperty(dlg, fields[i], "text")));
    engine.invoke(dlg, "close");
    engine.sleep(500);
    engine.print("Geometry of " + rowObjectName + ": " + geom);
    return geom;
}

var main_geom = layerGeometry("wgtRowDelegate_0000");
var seg_geom = layerGeometry(findRow("MRIcrop-seg-hippoR"));
engine.validateValue(seg_geom[0], 0.4, 1e-3);

//=== Reslice the overlay with the registration dialog
engine.trigger("actionRegistration");
engine.sleep(1000);
var regpanel = engine.findChild(mainwin, "RegistrationDialog");
engine.clickChild(regpanel, "btnReslice");
engine.sleep(1000);
var optdlg = engine.findChild(regpanel, "dlgResliceOptions");
engine.invoke(optdlg, "accept");
engine.sleep(3000);

// Find the Layer Inspector row of the layer with the given nickname prefix
function findRow(prefix)
{
    engine.trigger("actionLayerInspector");
    engine.sleep(500);
    var dlg = engine.findChild(mainwin, "dlgLayerInspector");
    var found = null;
    for (var r = 0; r < 10 && found === null; r++)
    {
        var name = "wgtRowDelegate_000" + r;
        var row = engine.findChild(dlg, name);
        if (!row)
            continue;
        var nick = String(engine.getChildProperty(row, "outLayerNickname", "text"));
        engine.print("Row " + r + ": " + nick);
        if (nick.indexOf(prefix) == 0)
            found = name;
    }
    engine.invoke(dlg, "close");
    engine.sleep(500);
    if (found === null)
        engine.testFailed("No layer named '" + prefix + "...' was found");
    return found;
}

//=== The resliced image is added as a new layer, on the main image's grid
var resliced_geom = layerGeometry(findRow("resliced"));
for (var i = 0; i < 6; i++)
    engine.validateValue(resliced_geom[i], main_geom[i], 1e-3);
