// Read the function library
include("Library");

// Reloading the active segmentation (which defines the reference space) from
// disk while another segmentation with a different grid is loaded. The other
// layers refer to the reloaded image as their reference space, so this used to
// read freed memory and crash. The cursor must stay where it was.

openMainImage("MRIcrop-orig.gipl.gz");
loadLabelDescriptions("MRIcrop-seg.label");
openSegmentation("MRIcrop-seg-hippoR-04mm.nii.gz");            // row 1
openAdditionalSegmentation("MRIcrop-seg-hippoL-04mm.nii.gz");  // row 2

// Open the Layer Inspector on the given row (which also makes a segmentation
// row the active segmentation). Optionally reload the layer from disk, then
// read the cursor position in that layer's own voxel coordinates.
function cursorInLayer(rowObjectName, reload)
{
    engine.trigger("actionLayerInspector");
    engine.sleep(500);
    var dlg = engine.findChild(mainwin, "dlgLayerInspector");
    var row = engine.findChild(dlg, rowObjectName);
    engine.setProperty(row, "selected", true);
    engine.sleep(500);

    if (reload)
    {
        engine.trigger("actionReloadFromFile", row);
        engine.sleep(2000);
    }

    engine.callChildMethod(dlg, "tabWidget", "setCurrentWidget", [engine.findChild(dlg, "cmpInfo")]);
    engine.sleep(500);
    var pos = [ String(engine.getChildProperty(dlg, "outVoxelObliqueX", "text")),
                String(engine.getChildProperty(dlg, "outVoxelObliqueY", "text")),
                String(engine.getChildProperty(dlg, "outVoxelObliqueZ", "text")) ];
    engine.invoke(dlg, "close");
    engine.print("Cursor in " + rowObjectName + (reload ? " after reload: " : ": ") + pos);
    return pos;
}

//=== Put the cursor at the centroid of hippo-R (in hippo-R's grid)
engine.trigger("actionActivatePreviousSegmentationLayer");  // hippo-R active
engine.sleep(500);
setCursor(31, 62, 40);
engine.sleep(500);
engine.validateChildProperty(mainwin, "outLabelText", "text", "hippo-R");
var before = cursorInLayer("wgtRowDelegate_0001", false);

//=== Reload hippo-R from disk: no crash, and the cursor stays in place
var after = cursorInLayer("wgtRowDelegate_0001", true);
for (var d = 0; d < 3; d++)
    engine.validateValue(after[d], before[d]);
engine.validateChildProperty(mainwin, "outLabelText", "text", "hippo-R");

//=== Switching to hippo-L and back still works after the reload
engine.trigger("actionActivateNextSegmentationLayer");  // hippo-L
engine.sleep(1000);
engine.trigger("actionActivatePreviousSegmentationLayer");  // hippo-R
engine.sleep(1000);
setCursor(31, 62, 40);
engine.sleep(500);
engine.validateChildProperty(mainwin, "outLabelText", "text", "hippo-R");
