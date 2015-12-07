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
engine.validateValue(value2, 604)

var value3 = readVoxelIntensity(2);
engine.validateValue(value3, 554)

// TODO: validate registration?
