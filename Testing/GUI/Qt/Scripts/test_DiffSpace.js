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
engine.validateValue(value2, 608)

var value3 = readVoxelIntensity(2);
engine.validateValue(value3, 554)

//=== Opening registration panel
engine.findChild(mainwin,"actionRegistration").trigger();
var regpanel = engine.findChild(mainwin,"RegistrationDialog");

//=== Run automatic registration
engine.findChild(regpanel, "btnRunRegistration").click();
engine.sleep(2000);

//=== Check a rotation and a translation
var rot_x = engine.findChild(regpanel, "inRotX").value;
var trn_z = engine.findChild(regpanel, "inTranZ").value;

engine.validateFloatValue(rot_x, 3.29, 0.1);
engine.validateFloatValue(trn_z, -0.722, 0.02);



