// Read the function library
include("Library");

function getRandomInt(min, max) {
    return min + Math.floor(Math.random() * max);
}


//=== Open the test workspace
openWorkspace("img4d_11f.itksnap");

//=== Find continuous update and triggers
var actionContUpdate = engine.findChild(mainwin,"actionContinuous_Update");
actionContUpdate.trigger();

engine.sleep(1000);

//=== scroll through frames
for (let i = 0; i < 12; i++) {
    setCursor4D(15, 23, 12, i);
}

for (let i = 11; i > 0; i--) {
    setCursor4D(16,25, 11, i);
}

//=== Random frames
for (let i = 0; i < 25; i++) {
    let f = getRandomInt(1, 11);
    setCursor4D(15, 23, 12, f);
}

engine.sleep(1000);

//=== 4D Playing
// Open and select the layer inspector dialog
engine.findChild(mainwin,"actionLayerInspector").trigger();
var layerdialog = engine.findChild(mainwin,"dlgLayerInspector");

// Select the 4D Property Group
var grp4D = engine.findChild(mainwin, "grp4DProperties");
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



