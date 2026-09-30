// Read the function library
include("Library");

//=== Load the main image and the shared label description file
openMainImage("MRIcrop-orig.gipl.gz");
loadLabelDescriptions("MRIcrop-seg.label");

//=== Load hippo-R (label 4) as THE segmentation (replaces the auto-created
//=== blank segmentation). It becomes active.
openSegmentation("MRIcrop-seg-hippoR-04mm.nii.gz");

var maxR = {
    x: engine.getChildProperty(mainwin, "inCursorX", "maximum"),
    y: engine.getChildProperty(mainwin, "inCursorY", "maximum"),
    z: engine.getChildProperty(mainwin, "inCursorZ", "maximum")
};

//=== Cursor at the centroid of the active (hippo-R) segmentation's label mask
//=== should read that label's name
setCursor(31, 62, 40);  // centroid of the hippo-R label mask
engine.sleep(500);
engine.validateChildProperty(mainwin, "outLabelText", "text", "hippo-R");

//=== Layer Inspector should report hippo-R's own 0.4mm resolution
//=== (row 0 = main, row 1 = hippo-R, replacing the auto-created blank segmentation)
var infoR = getLayerResolutionInfo("wgtRowDelegate_0001");
engine.validateValue(infoR.spacingX, 0.4, 0.001);
engine.validateValue(infoR.spacingY, 0.4, 0.001);
engine.validateValue(infoR.spacingZ, 0.4, 0.001);
engine.closeChild(mainwin, "dlgLayerInspector")

//=== 3D mesh render should succeed for hippo-R
updateMeshAndCheck();

//=== Load hippo-L (label 5) additively. It becomes active.
openAdditionalSegmentation("MRIcrop-seg-hippoL-04mm.nii.gz");

var maxL = {
    x: engine.getChildProperty(mainwin, "inCursorX", "maximum"),
    y: engine.getChildProperty(mainwin, "inCursorY", "maximum"),
    z: engine.getChildProperty(mainwin, "inCursorZ", "maximum")
};

setCursor(49, 67, 30);  // centroid of the hippo-L label mask
engine.sleep(500);
engine.validateChildProperty(mainwin, "outLabelText", "text", "hippo-L");

//=== Layer Inspector should report hippo-L's own 0.4mm resolution (row 2 = hippo-L)
var infoL = getLayerResolutionInfo("wgtRowDelegate_0002");
engine.validateValue(infoL.spacingX, 0.4, 0.001);
engine.validateValue(infoL.spacingY, 0.4, 0.001);
engine.validateValue(infoL.spacingZ, 0.4, 0.001);
engine.closeChild(mainwin, "dlgLayerInspector")

updateMeshAndCheck();

//=== Switch back to hippo-R with '{' and confirm geometry/labels re-sync correctly
engine.trigger("actionActivatePreviousSegmentationLayer");
engine.sleep(500);

engine.validateChildProperty(mainwin, "inCursorX", "maximum", maxR.x);
engine.validateChildProperty(mainwin, "inCursorY", "maximum", maxR.y);
engine.validateChildProperty(mainwin, "inCursorZ", "maximum", maxR.z);

setCursor(31, 62, 40);  // centroid of the hippo-R label mask
engine.sleep(500);
engine.validateChildProperty(mainwin, "outLabelText", "text", "hippo-R");

//=== hippo-R's row-level info should be unaffected by which layer is active
var infoR2 = getLayerResolutionInfo("wgtRowDelegate_0001");
engine.validateValue(infoR2.dimX, infoR.dimX);
engine.validateValue(infoR2.dimY, infoR.dimY);
engine.validateValue(infoR2.dimZ, infoR.dimZ);
engine.closeChild(mainwin, "dlgLayerInspector")

updateMeshAndCheck();

//=== Switch forward to hippo-L with '}' and confirm the same
engine.trigger("actionActivateNextSegmentationLayer");
engine.sleep(500);

engine.validateChildProperty(mainwin, "inCursorX", "maximum", maxL.x);
engine.validateChildProperty(mainwin, "inCursorY", "maximum", maxL.y);
engine.validateChildProperty(mainwin, "inCursorZ", "maximum", maxL.z);

setCursor(49, 67, 30);  // centroid of the hippo-L label mask
engine.sleep(500);
engine.validateChildProperty(mainwin, "outLabelText", "text", "hippo-L");

updateMeshAndCheck();
