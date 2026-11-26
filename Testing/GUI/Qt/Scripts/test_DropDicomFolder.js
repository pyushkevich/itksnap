// Read the function library
include("Library");

// Dropping a folder of DICOM files (#147). The drop dialog used to read the
// header of the dropped file to set up its buttons, which fails for a folder,
// so nothing was loaded. A folder now opens the Open Image wizard on its
// list of DICOM series. The test exam has three series (see
// test_DicomSeriesSelection.js).
var folder = datadir + "/dicom_ct_exam";

// In the wizard opened by a drop, check that it shows the series in the
// folder, choose one by its description and load it
function chooseSeriesInWizard(desc)
{
    var wiz = engine.findVisibleChild(mainwin, "wizImageIO");
    if (!wiz)
        engine.testFailed("Dropping a DICOM folder did not open the wizard");

    var table = engine.findChild(wiz, "tableDICOMSeries");
    engine.validateValue(engine.getProperty(table, "rowCount"), 3);
    var row = -1;
    for (var i = 0; i < 3; i++)
    {
        // DICOM pads strings to an even length, so trim the description
        if (engine.tableItemText(table, i, 1).trim() == desc)
            row = i;
    }
    if (row < 0)
        engine.testFailed("Series " + desc + " is not listed");

    engine.callMethod(table, "selectRow", [row], true);
    engine.sleep(500);
    engine.clickChild(wiz, "qt_wizard_commit");
    engine.sleep(2000);
    engine.clickChild(wiz, "qt_wizard_finish");
    engine.sleep(2000);
}

// Dimensions and intensity range of a layer in the Layer Inspector
function validateLayer(row, dimz, imin, imax)
{
    engine.trigger("actionLayerInspector");
    engine.sleep(500);
    var dlg = engine.findChild(mainwin, "dlgLayerInspector");
    engine.setProperty(engine.findChild(dlg, "wgtRowDelegate_000" + row), "selected", true);
    engine.callChildMethod(dlg, "tabWidget", "setCurrentWidget", [engine.findChild(dlg, "cmpInfo")]);
    engine.sleep(500);
    engine.validateValue(engine.getChildProperty(dlg, "outDimZ", "text"), dimz);
    engine.validateValue(engine.getChildProperty(dlg, "outMin", "text"), imin);
    engine.validateValue(engine.getChildProperty(dlg, "outMax", "text"), imax);
    engine.invoke(dlg, "close");
    engine.sleep(500);
}

//=== With no image loaded, a dropped folder opens the wizard on its series,
//=== and the chosen series becomes the main image (intensity 1000 + slice)
engine.callMethod(mainwin, "LoadDroppedFile", [folder, true]);
engine.sleep(3000);
chooseSeriesInWizard("Axial 3D A");
waitForMainImage();
validateLayer(0, "8", "1000", "1007");

//=== With an image loaded, a dropped folder shows the drop dialog, and
//=== loading it as an additional image goes through the wizard too
//=== (intensity 2000 + slice)
engine.callMethod(mainwin, "LoadDroppedFile", [folder, true]);
engine.sleep(1000);
var dropDialog = engine.findVisibleChild(mainwin, "DropActionDialog");
if (!dropDialog)
    engine.testFailed("Dropping a DICOM folder on a loaded image did not show the drop dialog");
engine.clickChild(dropDialog, "btnLoadOverlay");
engine.sleep(3000);
chooseSeriesInWizard("Axial 3D B");
validateLayer(1, "6", "2000", "2005");
