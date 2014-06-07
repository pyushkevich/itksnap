// This script positions the cursor and checks image intensity

// Call the open image script
thread.source("Library")

// Open the test image
openMainImage("MRIcrop-orig.gipl.gz");

// Put cursor somewhere
setCursor(75,6,27);

// Check the image intensity
var value = readVoxelIntensity(0);

// Validate
engine.validateValue(value, 54)
