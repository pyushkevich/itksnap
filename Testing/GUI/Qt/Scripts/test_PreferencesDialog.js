// Read the function library
include("Library");

// Show the preferences dialog
engine.findChild(mainwin,"actionPreferences").trigger();

// Get Preferences Dialog
var prefDialog = engine.findChild(mainwin, "PreferencesDialog");

// Toggle the radio buttons
// -- make sure the icon switch does not crash the program
var btnASC = engine.findChild(prefDialog, "btnASC");
var btnSAC = engine.findChild(prefDialog, "btnSAC");
btnASC.toggle();
btnSAC.toggle();
var btnAP = engine.findChild(prefDialog, "radio_sagittal_ap");
var btnPA = engine.findChild(prefDialog, "radio_sagittal_pa");
var btnRL = engine.findChild(prefDialog, "radio_axial_rl");
var btnLR = engine.findChild(prefDialog, "radio_axial_lr");
btnAP.toggle();
btnPA.toggle();
btnRL.toggle();
btnLR.toggle();



