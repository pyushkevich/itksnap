// Read the function library
include("Library");

function getRandomInt(min, max) {
    return min + Math.floor(Math.random() * max);
}

function scrollThroughTimePoints() {
    // scroll through frames
    for (let i = 0; i < 12; i++) {
        setCursor4D(15, 23, 12, i);
    }

    for (let i = 11; i > 0; i--) {
        setCursor4D(16,25, 11, i);
    }

    // Random frames
    for (let i = 0; i < 25; i++) {
        let f = getRandomInt(1, 11);
        setCursor4D(15, 23, 12, f);
    }

    engine.sleep(500);
}


//=== Open the test workspace
openWorkspace("segmentation_mesh.itksnap");

//=== Find continuous update and triggers
engine.trigger("actionContinuous_Update");
engine.sleep(1000);

//=== Show the layer inspector
engine.trigger("actionLayerInspector");

//=== Select a specific overlay
var layerdialog = engine.findChild(mainwin,"dlgLayerInspector");

var rowdelegate = engine.findChild(layerdialog, "wgtRowDelegate_0001");
rowdelegate.setSelected(true);
engine.sleep(500);

scrollThroughTimePoints();

var rowdelegate = engine.findChild(layerdialog, "wgtRowDelegate_0002");
rowdelegate.setSelected(true);
engine.sleep(500);

var rowdelegate = engine.findChild(layerdialog, "wgtRowDelegate_0003");
rowdelegate.setSelected(true);
engine.sleep(500);

scrollThroughTimePoints();

//=== Mesh layers should be generated
var rowdelegate = engine.findChild(layerdialog, "wgtRowDelegate_0004");
rowdelegate.setSelected(true);
engine.sleep(500);

scrollThroughTimePoints();

var rowdelegate = engine.findChild(layerdialog, "wgtRowDelegate_0005");
rowdelegate.setSelected(true);
engine.sleep(500);

var rowdelegate = engine.findChild(layerdialog, "wgtRowDelegate_0006");
rowdelegate.setSelected(true);
engine.sleep(500);


var rowdelegate = engine.findChild(layerdialog, "wgtRowDelegate_0000");
rowdelegate.setSelected(true);
engine.sleep(500);


//=== 4D Playing
// Open and select the layer inspector dialog
// Select the 4D Property Group
var grp4D = engine.findChild(layerdialog, "grp4DProperties");
if(!grp4D.visible)
    engine.testFailed("4D Property Group was not shown");

var btnReplay = engine.findChild(grp4D, "btn4DReplay");
btnReplay.click();
engine.sleep(2000);
btnReplay.click();

engine.findChild(grp4D, "in4DReplayInterval").text = "200"
btnReplay.click();
engine.sleep(3000);
btnReplay.click();

engine.findChild(grp4D, "in4DReplayInterval").text = "20"
btnReplay.click();
engine.sleep(2000);
btnReplay.click();



