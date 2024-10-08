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
btnExpand.click();

//=== Show the layer inspector
engine.trigger("actionLayerInspector");

//=== Select a specific overlay
var layerdialog = engine.findChild(mainwin,"dlgLayerInspector");
var rowdelegate = engine.findChild(layerdialog, "wgtRowDelegate_0002");
rowdelegate.setSelected(true);

//=== Select the mesh data name box
var boxDataArray = engine.findChild(layerdialog, "boxMeshDataName");

//=== Select all available array
for (let i = 0; i <= 16; ++i) {
  boxDataArray.setCurrentIndex(i);
  engine.sleep(200);
}

//=== Test vector mode for multi-component arrays
var boxVectorMode = engine.findChild(layerdialog, "boxMeshVectorMode");

for (let arrInd = 3; arrInd <= 4; ++arrInd) {
    boxDataArray.setCurrentIndex(3);

    for (let i = 0; i <= 3; ++i){
        boxVectorMode.setCurrentIndex(i);
        engine.sleep(200);
    }
}

//=== Go to the color map widget
var cmpcolormap = engine.findChild(layerdialog, "cmpColorMap");
engine.findChild(layerdialog, "tabWidget").setCurrentWidget(cmpcolormap);

//=== Select the color map preset we want
var inpreset = engine.findChild(cmpcolormap, "inPreset");
inpreset.setCurrentIndex(8);
