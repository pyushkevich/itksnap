include("Library");

//=== Open a main image
openMainImage("img4d_11f.nii.gz");

//=== Open a mesh
openMesh("external_mesh_f1.vtp");

engine.sleep(1000);

//=== Expand 3D Panel
var panel3D = engine.findChild(mainwin, "panel3D");
engine.printChildren(panel3D);
var btnExpand = engine.findChild(panel3D, "btnExpand");
engine.click(btnExpand);

//=== Show the layer inspector
engine.trigger("actionLayerInspector");

//=== Select a specific overlay
var layerdialog = engine.findChild(mainwin,"dlgLayerInspector");
var rowdelegate = engine.findChild(layerdialog, "wgtRowDelegate_0002");
engine.setProperty(rowdelegate, "selected", true);

//=== Select the mesh data name box
var boxDataArray = engine.findChild(layerdialog, "boxMeshDataName");

//=== Select all available array
for (let i = 0; i <= 16; ++i) {
  engine.callMethod(boxDataArray, "setCurrentIndex", [i]);
  engine.sleep(200);
}

//=== Test vector mode for multi-component arrays
var boxVectorMode = engine.findChild(layerdialog, "boxMeshVectorMode");

for (let arrInd = 3; arrInd <= 4; ++arrInd) {
    engine.callMethod(boxDataArray, "setCurrentIndex", [3]);

    for (let i = 0; i <= 3; ++i){
        engine.callMethod(boxVectorMode, "setCurrentIndex", [i]);
        engine.sleep(200);
    }
}

//=== Go to the color map widget
var cmpcolormap = engine.findChild(layerdialog, "cmpColorMap");
engine.callChildMethod(layerdialog, "tabWidget", "setCurrentWidget", [cmpcolormap]);

//=== Select the color map preset we want
var inpreset = engine.findChild(cmpcolormap, "inPreset");
engine.callMethod(inpreset, "setCurrentIndex", [8]);
