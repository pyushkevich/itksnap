// Read the function library
include("Library");

// Test of the cursor history (#157): the Back and Forward buttons next to
// "zoom to fit" in the slice views, Reset View in the Navigation menu, and the
// menu of the zoom to fit button. Restoring the cursor when an image is
// reopened is checked by the CursorRestore test, since it is turned off when
// ITK-SNAP runs a test.

// A new place in the history starts when the cursor has been still for a
// second, so we wait a bit longer than that between moves
var PAUSE = 1500;

// Read the cursor position from the Cursor Inspector
function getCursor()
{
    return [engine.getChildProperty(mainwin, "inCursorX", "value"),
            engine.getChildProperty(mainwin, "inCursorY", "value"),
            engine.getChildProperty(mainwin, "inCursorZ", "value")];
}

function validateCursor(expected, what)
{
    var c = getCursor();
    if (c[0] != expected[0] || c[1] != expected[1] || c[2] != expected[2])
        engine.testFailed(what + ": cursor at " + c + ", expected " + expected);
    engine.print(what + ": cursor at " + c + " ok");
}

function validateEnabled(name, expected)
{
    engine.validateValue(engine.getChildProperty(panel, name, "enabled"), expected);
}

function getCommonZoom()
{
    return engine.getChildProperty(mainwin, "inZoom", "value");
}

var panel = engine.findChild(mainwin, "panel0");

//=== Load the main image. It opens at its center, with nothing to go back to
openMainImage("MRIcrop-orig.gipl.gz");
engine.sleep(PAUSE);
var center = getCursor();
validateEnabled("btnCursorBack", false);
validateEnabled("btnCursorForward", false);

//=== Move the cursor to two places, pausing after each
var p1 = [10, 20, 30], p2 = [20, 30, 40];
setCursor(p1[0], p1[1], p1[2]);
engine.sleep(PAUSE);
setCursor(p2[0], p2[1], p2[2]);
engine.sleep(PAUSE);
validateEnabled("btnCursorBack", true);
validateEnabled("btnCursorForward", false);

//=== Go back twice, to the center where the image opened
engine.clickChild(panel, "btnCursorBack");
engine.sleep(300);
validateCursor(p1, "Back");
engine.clickChild(panel, "btnCursorBack");
engine.sleep(300);
validateCursor(center, "Back again");
validateEnabled("btnCursorBack", false);
validateEnabled("btnCursorForward", true);

//=== Go forward twice, to the last place
engine.clickChild(panel, "btnCursorForward");
engine.sleep(300);
validateCursor(p1, "Forward");
engine.clickChild(panel, "btnCursorForward");
engine.sleep(300);
validateCursor(p2, "Forward again");
validateEnabled("btnCursorForward", false);

//=== Turn on linked zoom so that the common zoom level can be read back
//=== (it is a saved preference, so remember the user's setting)
var linkedZoomBefore = engine.getChildProperty(mainwin, "chkLinkedZoom", "checked");
engine.setChildProperty(mainwin, "chkLinkedZoom", "checked", true);
engine.sleep(500);
engine.trigger("actionZoomToFitInAllViews");
engine.sleep(500);
var zoomFit = getCommonZoom();

//=== Reset View puts the cursor back in the center and zooms to fit, and it
//=== can be undone with Back
engine.trigger("actionZoom_to_400");
engine.sleep(500);
engine.trigger("actionResetView");
engine.sleep(500);
validateCursor(center, "Reset View");
engine.validateValue(getCommonZoom(), zoomFit, 1e-3);
engine.clickChild(panel, "btnCursorBack");
engine.sleep(300);
validateCursor(p2, "Back after Reset View");

//=== Load a segmentation on a smaller, finer grid. The menu of the zoom to
//=== fit button then offers it as a separate fit target
openSegmentation("MRIcrop-seg-hippoR-04mm.nii.gz");
engine.sleep(500);
engine.invoke(panel, "UpdateZoomMenu");
var items = engine.menuActions(engine.findChild(panel, "btnZoomToFit"));
var texts = [];
for (var i = 0; i < items.length; i++)
    texts.push(items[i].text + (items[i].enabled ? "" : " (disabled)"));
engine.print("Zoom menu: " + texts.join(", "));
engine.validateValue(items[0].enabled, true);
engine.validateValue(items[2].enabled, true);

//=== Fitting the segmentation zooms in further than fitting the whole scene
engine.trigger("actionFitScene", panel);
engine.sleep(500);
var zoomScene = getCommonZoom();
engine.trigger("actionFitSegmentation", panel);
engine.sleep(500);
var zoomSeg = getCommonZoom();
engine.print("Zoom to fit the scene: " + zoomScene + ", the segmentation: " + zoomSeg);
if (!(zoomSeg > zoomScene))
    engine.testFailed("Fitting the segmentation should zoom in further than fitting the scene");

//=== Restore the user's linked zoom setting
engine.setChildProperty(mainwin, "chkLinkedZoom", "checked", linkedZoomBefore);
engine.sleep(500);
