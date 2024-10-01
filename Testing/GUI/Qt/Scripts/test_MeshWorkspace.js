// Read the function library
include("Library");

// Open the test workspace
openWorkspace("mesh_workspace.itksnap");

engine.sleep(1000);

//=== Expand 3D Panel
var panel3D = engine.findChild(mainwin, "panel3D");
var btnExpand = engine.findChild(panel3D, "btnExpand");
btnExpand.click();


//=== Select a specific overlay
var layerdialog = engine.findChild(mainwin,"dlgLayerInspector");
var rowdelegate = engine.findChild(layerdialog, "wgtRowDelegate_0002");
rowdelegate.setSelected(true);

//=== Show the layer inspector
engine.trigger("actionLayerInspector");

//=== Select the mesh data name box
var boxDataArray = engine.findChild(layerdialog, "boxMeshDataName");

//=== Close the inspector dialog
engine.invoke(layerdialog, "close");

//=== Validate data array settings
for (let i = 0; i < 17; ++i){
    boxDataArray.setCurrentIndex(i);
    let cmpcolormap = engine.findChild(layerdialog, "cmpColorMap");
    engine.findChild(layerdialog, "tabWidget").setCurrentWidget(cmpcolormap);

    let inpreset = engine.findChild(cmpcolormap, "inPreset");
    engine.validateValue(inpreset.currentIndex, i);
}

boxDataArray.setCurrentIndex(3);
let cmpContrast = engine.findChild(layerdialog, "cmpContrast");
let inControlId = engine.findChild(cmpContrast, "inControlId");

engine.validateValue(inControlId.maximum, 7);

//=== Close a specific mesh layer
engine.trigger("actionClose", rowdelegate);
engine.sleep(1000);

//=== The layer should be closed
let meshRowDelegate = engine.findChild(layerdialog, "wgtRowDelegate_0002");
engine.validateValue(meshRowDelegate, null);

//=== Trigger the unload all action
engine.trigger("actionUnload_All");
