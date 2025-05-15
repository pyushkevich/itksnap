// Read the function library
include("Library");

// Show the preferences dialog
engine.trigger("actionPreferences");

// Get Preferences Dialog
var prefDialog = engine.findChild(mainwin, "PreferencesDialog");
engine.invoke(prefDialog, "set_page_to_slice_views_layout")

// Toggle the radio buttons
// -- make sure the icon switch does not crash the program
var btnASC = engine.findChild(prefDialog, "btnASC");
var btnSAC = engine.findChild(prefDialog, "btnSAC");

//=== Toggle ASC
btnASC.toggle();

//=== Toggle SAC
btnSAC.toggle();

var btnAP = engine.findChild(prefDialog, "radio_sagittal_ap");
var btnPA = engine.findChild(prefDialog, "radio_sagittal_pa");
var btnRL = engine.findChild(prefDialog, "radio_axial_rl");
var btnLR = engine.findChild(prefDialog, "radio_axial_lr");

//=== Toggle AP
btnAP.toggle();

//=== Toggle PA
btnPA.toggle();

//=== Toggle RL
btnRL.toggle();

//=== Toggle LR
btnLR.toggle();



