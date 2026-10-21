// Read the function library
include("Library");

// Slice slider range and slice text when the scene extends beyond the
// reference space (the active segmentation)

//=== Load the main image and a cropped, upsampled segmentation. The main
//=== image extends well beyond the segmentation, so the scene does too.
openMainImage("MRIcrop-orig.gipl.gz");
loadLabelDescriptions("MRIcrop-seg.label");
openSegmentation("MRIcrop-seg-hippoR-04mm.nii.gz");

// The cursor inspector shows the extent of the scene (1-based)
function sceneRange(axis)
{
    var name = "inCursor" + axis;
    return {
        min: engine.getChildProperty(mainwin, name, "minimum"),
        max: engine.getChildProperty(mainwin, name, "maximum")
    };
}

function cursor()
{
    return {
        X: engine.getChildProperty(mainwin, "inCursorX", "value"),
        Y: engine.getChildProperty(mainwin, "inCursorY", "value"),
        Z: engine.getChildProperty(mainwin, "inCursorZ", "value")
    };
}

// Find the image axis along which a panel's slider moves the cursor
function sliceAxis(panelName)
{
    var panel = engine.findChild(mainwin, panelName);
    var axes = ["X", "Y", "Z"];
    var props = ["minimum", "maximum"];
    for(var k = 0; k < props.length; k++)
    {
        var before = cursor();
        engine.setChildProperty(panel, "inSlicePosition", "value",
                                engine.getChildProperty(panel, "inSlicePosition", props[k]));
        for(var t = 0; t < 20; t++)
        {
            engine.sleep(100);
            var after = cursor();
            for(var a = 0; a < 3; a++)
                if(before[axes[a]] != after[axes[a]])
                    return axes[a];
        }
    }
    engine.testFailed("Slider in " + panelName + " does not move the cursor");
}

// Slice axes of the three views. In one of them (sagittal for this image)
// the slice axis is flipped relative to the display, which used to mirror
// the slider range.
var panels = [];
var names = ["panel0", "panel1", "panel2"];
for(var i = 0; i < names.length; i++)
{
    panels.push([names[i], sliceAxis(names[i])]);
    engine.print(names[i] + " slices along " + panels[i][1]);
}

function panelForAxis(axis)
{
    for(var i = 0; i < panels.length; i++)
        if(panels[i][1] == axis)
            return engine.findChild(mainwin, panels[i][0]);
    engine.testFailed("No view slices along " + axis);
}

// Wait for the cursor inspector to show a value along an axis, then validate
function waitForCursor(axis, value)
{
    for(var t = 0; t < 30; t++)
    {
        if(engine.getChildProperty(mainwin, "inCursor" + axis, "value") == value)
            break;
        engine.sleep(100);
    }
    engine.validateChildProperty(mainwin, "inCursor" + axis, "value", value);
}

// Check that each slider covers the scene, and that both ends can be reached
function checkSliders()
{
    for(var i = 0; i < panels.length; i++)
    {
        var panel = engine.findChild(mainwin, panels[i][0]);
        var axis = panels[i][1];
        var rng = sceneRange(axis);

        engine.validateValue(engine.getChildProperty(panel, "inSlicePosition", "minimum"), rng.min - 1);
        engine.validateValue(engine.getChildProperty(panel, "inSlicePosition", "maximum"), rng.max - 1);

        engine.setChildProperty(panel, "inSlicePosition", "value", rng.min - 1);
        waitForCursor(axis, rng.min);

        engine.setChildProperty(panel, "inSlicePosition", "value", rng.max - 1);
        waitForCursor(axis, rng.max);
    }
}

//=== The scene is larger than the segmentation along x and y
var rx = sceneRange("X"), ry = sceneRange("Y");
if(rx.max - rx.min + 1 <= 75 || ry.min >= 1)
    engine.testFailed("Expected the scene to extend beyond the segmentation along x and y");

checkSliders();

//=== Outside of the segmentation, the slice number matches the cursor
//=== inspector and lies outside of 1..n, where n is the number of slices in
//=== the segmentation (75 x 135 x 95 voxels)
var panelX = panelForAxis("X"), panelY = panelForAxis("Y"), panelZ = panelForAxis("Z");
engine.setChildProperty(panelY, "inSlicePosition", "value", ry.min - 1);
engine.sleep(200);
engine.validateChildProperty(panelY, "lblSliceInfo", "text", ry.min + " of 135");

engine.setChildProperty(panelX, "inSlicePosition", "value", rx.max - 1);
engine.sleep(200);
engine.validateChildProperty(panelX, "lblSliceInfo", "text", rx.max + " of 75");

//=== Make the full-size segmentation active. Now the scene is the reference
//=== space, and the slice numbers are within 1..n.
openAdditionalSegmentation("MRIcrop-seg.gipl.gz");
engine.sleep(500);

checkSliders();

setCursor(40, 50, 30);
engine.sleep(500);
engine.validateChildProperty(panelX, "lblSliceInfo", "text", "40 of 78");
engine.validateChildProperty(panelY, "lblSliceInfo", "text", "50 of 110");
engine.validateChildProperty(panelZ, "lblSliceInfo", "text", "30 of 64");

//=== Switch back to the cropped segmentation: the ranges must follow
engine.trigger("actionActivatePreviousSegmentationLayer");
engine.sleep(500);
engine.validateValue(sceneRange("X").max, rx.max);
checkSliders();
