// Read the function library
include("Library");


// Open the test workspace
openWorkspace("img4d_11f.itksnap");

// Find continuous update and triggers
var actionContUpdate = engine.findChild(mainwin,"actionContinuous_Update");
actionContUpdate.trigger();

// scroll through frames

engine.sleep(3000);
