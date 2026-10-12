// Read the function library
include("Library");

// Annotations are stored in main image voxel coordinates, and must stay at
// the same anatomical location when the active segmentation (the reference
// space) is a different grid. MRIcrop-annot.annot has three rulers in the
// sagittal plane, on main image slices x = 38 and x = 65, and two landmarks
// in the coronal plane, on slices y = 56 and y = 37 (1-based slice numbers,
// as in the cursor inspector).

//=== Main image, MRIcrop-seg (same grid as the main image) and the cropped,
//=== 0.4mm hippo-R segmentation
openMainImage("MRIcrop-orig.gipl.gz");
loadLabelDescriptions("MRIcrop-seg.label");
openSegmentation("MRIcrop-seg.gipl.gz");
openAdditionalSegmentation("MRIcrop-seg-hippoR-04mm.nii.gz");

function cursor()
{
    return {
        X: engine.getChildProperty(mainwin, "inCursorX", "value"),
        Y: engine.getChildProperty(mainwin, "inCursorY", "value"),
        Z: engine.getChildProperty(mainwin, "inCursorZ", "value")
    };
}

// Find the view whose slider moves the cursor along the given axis
function panelForAxis(axis)
{
    var names = ["panel0", "panel1", "panel2"];
    for (var i = 0; i < names.length; i++)
    {
        var panel = engine.findChild(mainwin, names[i]);
        var before = cursor()[axis];
        var lo = engine.getChildProperty(panel, "inSlicePosition", "minimum");
        var hi = engine.getChildProperty(panel, "inSlicePosition", "maximum");
        var target = (engine.getChildProperty(panel, "inSlicePosition", "value") == lo) ? hi : lo;
        engine.setChildProperty(panel, "inSlicePosition", "value", target);
        engine.sleep(300);
        if (cursor()[axis] != before)
            return panel;
    }
    engine.testFailed("No view slices along " + axis);
}

function activateSegmentation(prev)
{
    engine.trigger(prev ? "actionActivatePreviousSegmentationLayer"
                        : "actionActivateNextSegmentationLayer");
    engine.sleep(1000);
}

// Step through the annotations in the view that slices along 'axis' with the
// 'next annotation' button, and return the main image slices visited. When
// hippo-R is active, the cursor is read after switching to MRIcrop-seg, which
// carries the cursor over in physical space and shows it in main image voxels.
function visitAnnotations(axis, n_steps, hippo_active)
{
    var panel = views[axis];
    engine.setChildProperty(panel, "inSlicePosition", "value",
                            engine.getChildProperty(panel, "inSlicePosition", "minimum"));
    engine.sleep(300);

    var visited = {};
    for (var k = 0; k < n_steps; k++)
    {
        engine.trigger("actionAnnotationNext", panel);
        engine.sleep(500);
        if (hippo_active)
            activateSegmentation(true);
        visited[cursor()[axis]] = true;
        if (hippo_active)
            activateSegmentation(false);
    }

    var list = Object.keys(visited).map(Number).sort(function(a, b) { return a - b; });
    engine.print(axis + " slices visited" + (hippo_active ? " with hippo-R active: " : ": ") + list);
    return list.join(",");
}

//=== Make MRIcrop-seg active, and find the sagittal (x) and coronal (y) views
activateSegmentation(true);
var views = { X: panelForAxis("X"), Y: panelForAxis("Y") };

//=== Load the annotations in annotation mode
engine.trigger("actionAnnotation");
engine.sleep(500);
engine.clickChild(engine.findChild(mainwin, "pageAnnotationTool"), "btnOpen");
engine.sleep(2000);
var dlg = engine.findVisibleChild(mainwin, "dlgSimpleFile");
if (!dlg)
    engine.testFailed("No file dialog for opening annotations");
engine.setChildProperty(dlg, "inFilename", "text", datadir + "/MRIcrop-annot.annot");
engine.invoke(dlg, "accept");
engine.sleep(2000);

//=== With the main image grid as reference
engine.validateValue(visitAnnotations("X", 3, false), "38,65");
engine.validateValue(visitAnnotations("Y", 2, false), "37,56");

//=== With hippo-R as reference, the annotations are on the same slices of
//=== the main image
activateSegmentation(false);
engine.validateValue(visitAnnotations("X", 3, true), "38,65");
engine.validateValue(visitAnnotations("Y", 2, true), "37,56");
