// Read the function library
include("Library");

// Open the test workspace
openWorkspace("diffspace.itksnap");

//=== Set Preferences to use linear interpolation
engine.trigger("actionPreferences");
var prefDialog = engine.findChild(mainwin, "PreferencesDialog");
engine.invoke(prefDialog, "set_page_to_slice_views_display")
var inInterp = engine.findChild(prefDialog, "inInterpolationMode");

// Set the value of the interpolation mode
engine.setProperty(inInterp, "currentText", "Linear");

// Accept the dialog
engine.invoke(prefDialog, "accept");

//=== Probe the image intensities at one location
setCursor(21,33,14);

// Check the image intensity
var value1 = readVoxelIntensity(0);
engine.validateValue(value1, 31)

var value2 = readVoxelIntensity(1);
engine.validateValue(value2, 608, 10)

var value3 = readVoxelIntensity(2);
engine.validateValue(value3, 554, 10)

//=== Opening registration panel
engine.trigger("actionRegistration");
var regpanel = engine.findChild(mainwin,"RegistrationDialog");

//=== Run automatic registration
engine.setChildProperty(regpanel, "inMovingLayer", "currentText", "t2_chunk");
engine.clickChild(regpanel, "btnRunRegistration");
engine.sleep(5000);

//=== Play with the multi_chunk, make sure it can be resliced
engine.print(engine.getChildProperty(regpanel, "tabAutoManual", "currentIndex"));
engine.print(engine.getChildProperty(regpanel, "tabAutoManual", "currentTabText"));
engine.setChildProperty(regpanel, "tabAutoManual", "currentIndex", 1);
engine.setChildProperty(regpanel, "inMovingLayer", "currentText", "multi_chunk");

//=== Set manual registration parameters
engine.setChildProperty(regpanel, "inRotX", "value", -52.0);
engine.setChildProperty(regpanel, "inRotY", "value", 46.0);
engine.setChildProperty(regpanel, "inTranX", "value", 12.0);
engine.setChildProperty(regpanel, "inTranY", "value", 8.0);
engine.setChildProperty(regpanel, "inTranZ", "value", -26.0);
engine.sleep(1000);

var value4 = readVoxelIntensity(2);
engine.validateValue(value4, 513, 10)

// Probe the image intensity outside of the image range
setCursor(4,17,14);

var value5 = readVoxelIntensity(2);
engine.validateValue(value5, 0);
