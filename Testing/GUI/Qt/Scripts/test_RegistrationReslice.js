// Read the function library
include("Library");

// Reslicing the moving image in the registration dialog must produce an image
// on the main image's grid (the registration is relative to the main image),
// even when the active segmentation, which defines the reference space, has
// a different grid.

openMainImage("MRIcrop-orig.gipl.gz");

//=== Make a segmentation on a 0.4mm grid the active one (the reference space)
openSegmentation("MRIcrop-seg-hippoR-04mm.nii.gz");

//=== Load the same image again as an overlay, to serve as the moving image
openOverlay("MRIcrop-orig.gipl.gz");

//=== The new overlay must show the slice at the cursor right away (it used to
//=== stay at slice 0 until the cursor was moved), so the voxel under the
//=== cursor is the same in both copies of the image
engine.sleep(500);
var v_main = readVoxelIntensity(0);
var v_overlay = readVoxelIntensity(1);
engine.print("Voxel under cursor: main = " + v_main + ", overlay = " + v_overlay);
engine.validateValue(v_overlay, v_main);

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

//=== Rotate the overlay in the registration dialog, then reslice it
engine.trigger("actionRegistration");
engine.sleep(1000);
var regpanel = engine.findChild(mainwin, "RegistrationDialog");
engine.setChildProperty(regpanel, "tabAutoManual", "currentIndex", 1);
engine.setChildProperty(regpanel, "inRotZ", "value", -36.0);
engine.sleep(1000);
var v_rotated = readVoxelIntensity(1);
engine.print("Voxel under cursor in the rotated overlay: " + v_rotated);
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
var resliced_row = findRow("resliced");
var resliced_geom = layerGeometry(resliced_row);
engine.print("Main geometry: " + main_geom + ", resliced geometry: " + resliced_geom);

//=== The resliced image samples the rotated overlay, so the voxel under the
//=== cursor matches the rotated overlay (the resliced layer is row 2 of the
//=== voxel table, which lists image layers only)
takeScreenshot(tempdir + "/RegistrationReslice_result.png", mainwin);
var v_resliced = readVoxelIntensity(2);
engine.print("Voxel under cursor in the resliced image: " + v_resliced);
engine.validateValue(v_resliced, v_rotated, 1);

for (var i = 0; i < 6; i++)
    engine.validateValue(resliced_geom[i], main_geom[i], 1e-3);
