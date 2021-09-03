// Read the function library
include("Library");

function getRandomInt(min, max) {
    return min + Math.floor(Math.random * max);
}


// Open the test workspace
openWorkspace("img4d_11f.itksnap");

// Find continuous update and triggers
var actionContUpdate = engine.findChild(mainwin,"actionContinuous_Update");
actionContUpdate.trigger();

<<<<<<< HEAD
engine.sleep(1000);

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

=======
// scroll through frames

engine.sleep(3000);
>>>>>>> afe7e7c5... czi-102: testing scripts
