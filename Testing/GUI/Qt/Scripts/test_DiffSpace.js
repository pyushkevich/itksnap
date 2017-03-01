// Read the function library
include("Library");

// Open the test workspace
openWorkspace("diffspace.itksnap");

// Probe the image intensities at one location
setCursor(21,33,14);

// Check the image intensity
var value1 = readVoxelIntensity(0);
engine.validateValue(value1, 31)

var value2 = readVoxelIntensity(1);
engine.validateFloatValue(value2, 608, 10)

var value3 = readVoxelIntensity(2);
engine.validateFloatValue(value3, 554, 10)

//=== Opening registration panel
engine.findChild(mainwin,"actionRegistration").trigger();
var regpanel = engine.findChild(mainwin,"RegistrationDialog");

//=== Run automatic registration 
engine.findChild(regpanel, "btnRunRegistration").click();
engine.sleep(2000);

//=== Play with the multi_chunk, make sure it can be resliced
engine.findChild(regpanel, "inMovingLayer").value = 2;
engine.findChild(regpanel, "inRotY").value = 16.0;
engine.findChild(regpanel, "inTranZ").value = -12.0;

engine.sleep(50000);
var value4 = readVoxelIntensity(2);
engine.validateFloatValue(value4, 394, 10)

// Probe the image intensity outside of the image range
setCursor(4,17,14);
var value5 = readVoxelIntensity(2);
engine.validateValue(value5, 0);



