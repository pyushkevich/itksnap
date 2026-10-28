// Read the function library
include("Library");

// Show the preferences dialog
engine.trigger("actionPreferences");

// Get Preferences Dialog
var prefDialog = engine.findChild(mainwin, "PreferencesDialog");

// The crosshair (the element shown first) must keep its line type when the
// dialog is set up; filling the line type list used to set it to No Line
//=== Check the crosshair line type
engine.invoke(prefDialog, "set_page_to_appearance");
engine.sleep(500);
var lineType = engine.getChildProperty(prefDialog, "inElementLineType", "currentText");
if (lineType == "No Line")
    engine.testFailed("Crosshair line type was changed to No Line at startup");
engine.invoke(prefDialog, "set_page_to_slice_views_layout")

// Toggle the radio buttons
// -- make sure the icon switch does not crash the program
var btnASC = engine.findChild(prefDialog, "btnASC");
var btnSAC = engine.findChild(prefDialog, "btnSAC");

//=== Toggle ASC
engine.toggle(btnASC);

//=== Toggle SAC
engine.toggle(btnSAC);

var btnAP = engine.findChild(prefDialog, "radio_sagittal_ap");
var btnPA = engine.findChild(prefDialog, "radio_sagittal_pa");
var btnRL = engine.findChild(prefDialog, "radio_axial_rl");
var btnLR = engine.findChild(prefDialog, "radio_axial_lr");

//=== Toggle AP
engine.toggle(btnAP);

//=== Toggle PA
engine.toggle(btnPA);

//=== Toggle RL
engine.toggle(btnRL);

//=== Toggle LR
engine.toggle(btnLR);


