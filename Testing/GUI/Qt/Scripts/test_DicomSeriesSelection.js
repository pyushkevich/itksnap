// Read the function library
include("Library");

// Choosing a series from a CT exam with several series (#218). The test exam
// has two 3D series and one 4D CTA series from a GE scanner; it used to be
// opened as 4D CTA as a whole, loading the first series without a choice.

// Open the exam from a file of series 2, pick a series by its description in
// the wizard's series list and load it
function openDicomSeries(desc)
{
    engine.trigger("actionOpenMain");
    engine.sleep(2000);

    var wiz = engine.findChild(mainwin, "wizImageIO");
    engine.setChildProperty(wiz, "inFilename", "text", datadir + "/dicom_ct_exam/s2_0001.dcm");
    engine.sleep(500);
    engine.validateValue(engine.getChildProperty(wiz, "inFormat", "currentText"), "DICOM Image Series");

    //=== Go to the series page
    engine.clickChild(wiz, "qt_wizard_commit");
    engine.sleep(2000);

    //=== All three series should be listed; find the one we want
    var table = engine.findChild(wiz, "tableDICOMSeries");
    engine.validateValue(engine.getProperty(table, "rowCount"), 3);
    var row = -1;
    for (var i = 0; i < 3; i++)
    {
        // DICOM pads strings to an even length, so trim the description
        var d = engine.tableItemText(table, i, 1).trim();
        if (d == "Dynamic CTA")
            engine.validateValue(engine.tableItemText(table, i, 2), "16 x 16 x 5 x 3");
        if (d == desc)
            row = i;
    }
    if (row < 0)
        engine.testFailed("Series " + desc + " is not listed");

    //=== Select the series and load it
    engine.callMethod(table, "selectRow", [row], true);
    engine.sleep(500);
    engine.clickChild(wiz, "qt_wizard_commit");
    engine.sleep(2000);
    engine.clickChild(wiz, "qt_wizard_finish");
    engine.sleep(2000);
}

// Check the dimensions and intensity range of the main image
function validateMainImage(dimz, dimt, imin, imax)
{
    engine.trigger("actionLayerInspector");
    engine.sleep(500);
    var dlg = engine.findChild(mainwin, "dlgLayerInspector");
    engine.callChildMethod(dlg, "tabWidget", "setCurrentWidget", [engine.findChild(dlg, "cmpInfo")]);
    engine.sleep(500);

    engine.validateValue(engine.getChildProperty(dlg, "outDimX", "text"), "16");
    engine.validateValue(engine.getChildProperty(dlg, "outDimY", "text"), "16");
    engine.validateValue(engine.getChildProperty(dlg, "outDimZ", "text"), dimz);
    engine.validateValue(engine.getChildProperty(dlg, "outDimT", "text"), dimt);
    engine.validateValue(engine.getChildProperty(dlg, "outMin", "text"), imin);
    engine.validateValue(engine.getChildProperty(dlg, "outMax", "text"), imax);

    engine.invoke(dlg, "close");
}

//=== The 4D CTA series loads as 5 slices x 3 frames (intensity 100 * frame + slice)
openDicomSeries("Dynamic CTA");
validateMainImage("5", "3", "0", "204");

//=== A 3D series other than the first one can be chosen (intensity 2000 + slice)
openDicomSeries("Axial 3D B");
validateMainImage("6", "1", "2000", "2005");
