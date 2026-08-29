// Read the function library
include("Library");

//=== Load the main image and the shared label description file
openMainImage("MRIcrop-orig.gipl.gz");
loadLabelDescriptions("MRIcrop-seg.label");

//=== Load first additional segmentation (hippo-R, label 4). It becomes active.
openAdditionalSegmentation("MRIcrop-seg-hippoR-04mm.nii.gz");

var maxR = {
    x: engine.findChild(mainwin, "inCursorX").maximum,
    y: engine.findChild(mainwin, "inCursorY").maximum,
    z: engine.findChild(mainwin, "inCursorZ").maximum
};

//=== Cursor at the centroid of the active (hippo-R) segmentation's label mask
//=== should read that label's name
setCursor(31, 62, 40);  // centroid of the hippo-R label mask
engine.sleep(500);
engine.validateValue(engine.findChild(mainwin, "outLabelText").text, "hippo-R");

//=== Layer Inspector should report hippo-R's own 0.4mm resolution (row 1 = 1st segmentation)
var infoR = getLayerResolutionInfo("wgtRowDelegate_0001");
engine.validateFloatValue(infoR.spacingX, 0.4, 0.001);
engine.validateFloatValue(infoR.spacingY, 0.4, 0.001);
engine.validateFloatValue(infoR.spacingZ, 0.4, 0.001);

//=== 3D mesh render should succeed for hippo-R
updateMeshAndCheck();

//=== Load second additional segmentation (hippo-L, label 5). It becomes active.
openAdditionalSegmentation("MRIcrop-seg-hippoL-04mm.nii.gz");

var maxL = {
    x: engine.findChild(mainwin, "inCursorX").maximum,
    y: engine.findChild(mainwin, "inCursorY").maximum,
    z: engine.findChild(mainwin, "inCursorZ").maximum
};

setCursor(49, 67, 30);  // centroid of the hippo-L label mask
engine.sleep(500);
engine.validateValue(engine.findChild(mainwin, "outLabelText").text, "hippo-L");

//=== Layer Inspector should report hippo-L's own 0.4mm resolution (row 2 = 2nd segmentation)
var infoL = getLayerResolutionInfo("wgtRowDelegate_0002");
engine.validateFloatValue(infoL.spacingX, 0.4, 0.001);
engine.validateFloatValue(infoL.spacingY, 0.4, 0.001);
engine.validateFloatValue(infoL.spacingZ, 0.4, 0.001);

updateMeshAndCheck();

//=== Switch back to hippo-R with '{' and confirm geometry/labels re-sync correctly
engine.trigger("actionActivatePreviousSegmentationLayer");
engine.sleep(500);

engine.validateValue(engine.findChild(mainwin, "inCursorX").maximum, maxR.x);
engine.validateValue(engine.findChild(mainwin, "inCursorY").maximum, maxR.y);
engine.validateValue(engine.findChild(mainwin, "inCursorZ").maximum, maxR.z);

setCursor(31, 62, 40);  // centroid of the hippo-R label mask
engine.sleep(500);
engine.validateValue(engine.findChild(mainwin, "outLabelText").text, "hippo-R");

//=== hippo-R's row-level info should be unaffected by which layer is active
var infoR2 = getLayerResolutionInfo("wgtRowDelegate_0001");
engine.validateValue(infoR2.dimX, infoR.dimX);
engine.validateValue(infoR2.dimY, infoR.dimY);
engine.validateValue(infoR2.dimZ, infoR.dimZ);

updateMeshAndCheck();

//=== Switch forward to hippo-L with '}' and confirm the same
engine.trigger("actionActivateNextSegmentationLayer");
engine.sleep(500);

engine.validateValue(engine.findChild(mainwin, "inCursorX").maximum, maxL.x);
engine.validateValue(engine.findChild(mainwin, "inCursorY").maximum, maxL.y);
engine.validateValue(engine.findChild(mainwin, "inCursorZ").maximum, maxL.z);

setCursor(49, 67, 30);  // centroid of the hippo-L label mask
engine.sleep(500);
engine.validateValue(engine.findChild(mainwin, "outLabelText").text, "hippo-L");

updateMeshAndCheck();
