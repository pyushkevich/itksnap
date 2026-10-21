// Read the function library
include("Library");

// Read the common (linked) zoom level from the Zoom Inspector
function getCommonZoom()
{
    return engine.getChildProperty(mainwin, "inZoom", "value");
}

function validateDifferent(a, b, what)
{
    if (Math.abs(a - b) < 1e-3)
        engine.testFailed(what + ": expected different zoom levels, got " + a + " and " + b);
    engine.print(what + ": " + a + " != " + b + " ok");
}

//=== Load the main image and a segmentation on a smaller, finer grid. The
//=== segmentation becomes the active one (and the reference space), so the
//=== fit targets are: whole scene (= main image here) and the segmentation
openMainImage("MRIcrop-orig.gipl.gz");
openSegmentation("MRIcrop-seg-hippoR-04mm.nii.gz");

//=== Turn on linked zoom so that the common zoom level can be read back
//=== (it is a saved preference, so remember the user's setting)
var linkedZoomBefore = engine.getChildProperty(mainwin, "chkLinkedZoom", "checked");
engine.setChildProperty(mainwin, "chkLinkedZoom", "checked", true);
engine.sleep(500);

//=== Zoom to fit in all views cycles between the two distinct targets
engine.trigger("actionZoomToFitInAllViews");
engine.sleep(500);
var z1 = getCommonZoom();
engine.trigger("actionZoomToFitInAllViews");
engine.sleep(500);
var z2 = getCommonZoom();
engine.trigger("actionZoomToFitInAllViews");
engine.sleep(500);
var z3 = getCommonZoom();
validateDifferent(z1, z2, "All views, presses 1 and 2");
engine.validateValue(z3, z1, 1e-3);

//=== After zooming in, the next press re-fits the same target (no advance)
engine.setChildProperty(mainwin, "inZoom", "value", z3 * 2);
engine.sleep(500);
engine.trigger("actionZoomToFitInAllViews");
engine.sleep(500);
engine.validateValue(getCommonZoom(), z3, 1e-3);

//=== The per-view zoom to fit button cycles in the same way
var panel = engine.findChild(mainwin, "panel0");
engine.clickChild(panel, "btnZoomToFit");
engine.sleep(500);
var p1 = getCommonZoom();
engine.clickChild(panel, "btnZoomToFit");
engine.sleep(500);
var p2 = getCommonZoom();
engine.clickChild(panel, "btnZoomToFit");
engine.sleep(500);
var p3 = getCommonZoom();
validateDifferent(p1, p2, "Per-view button, presses 1 and 2");
engine.validateValue(p3, p1, 1e-3);

//=== Restore the user's linked zoom setting
engine.setChildProperty(mainwin, "chkLinkedZoom", "checked", linkedZoomBefore);
