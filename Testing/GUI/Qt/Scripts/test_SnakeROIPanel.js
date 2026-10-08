// Read the function library
include("Library");

// Tests editing the snake ROI through the Index and Size spin boxes of the
// snake ROI panel. The panel shows 1-based voxel indices.

openMainImage("MRIcrop-orig.gipl.gz");

//=== Show the snake ROI panel and reset the ROI to the whole image
engine.trigger("actionSnake");
engine.sleep(500);
var roipanel = engine.findChild(mainwin, "pageSnakeTool");
engine.clickChild(roipanel, "btnResetROI");
engine.sleep(500);

function roiValue(name)
{
    return engine.getChildProperty(roipanel, name, "value");
}

var nx = roiValue("inSizeX"), ny = roiValue("inSizeY"), nz = roiValue("inSizeZ");
engine.print("Image size: " + nx + " x " + ny + " x " + nz);

//=== Moving the X index must keep the size on the other axes, and shrink
//=== the X size only as much as needed to stay inside the image
engine.setChildProperty(roipanel, "inIndexX", "value", 10);
engine.sleep(500);
engine.validateValue(roiValue("inIndexX"), 10);
engine.validateValue(roiValue("inSizeX"), nx - 9);
engine.validateValue(roiValue("inSizeY"), ny);
engine.validateValue(roiValue("inSizeZ"), nz);

//=== The last voxel can be chosen as the start of the ROI
engine.setChildProperty(roipanel, "inIndexZ", "value", nz);
engine.sleep(500);
engine.validateValue(roiValue("inIndexZ"), nz);
engine.validateValue(roiValue("inSizeZ"), 1);

//=== Growing the X size past the edge of the image must move the X index
//=== back (by the X size, not the Y size), keeping the ROI inside the image
engine.setChildProperty(roipanel, "inSizeY", "value", 20);
engine.sleep(500);
engine.setChildProperty(roipanel, "inSizeX", "value", nx);
engine.sleep(500);
engine.validateValue(roiValue("inSizeX"), nx);
engine.validateValue(roiValue("inIndexX"), 1);
engine.validateValue(roiValue("inSizeY"), 20);
