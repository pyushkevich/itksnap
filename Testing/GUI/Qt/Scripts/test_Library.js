function setCursor(x, y, z)
{
  engine.print("Setting cursor position to " + x + ", " + y + ", " + z);
  engine.setChildProperty(mainwin, "inCursorX", "value", x);
  engine.setChildProperty(mainwin, "inCursorY", "value", y);
  engine.setChildProperty(mainwin, "inCursorZ", "value", z);
  engine.sleep(200);
}
function setCursor4D(x, y, z, t)
{
    engine.print("Setting cursor position to " + x + ", " + y + ", " + z + ", " + t);
    engine.setChildProperty(mainwin, "inCursorX_4D", "value", x);
    engine.setChildProperty(mainwin, "inCursorY_4D", "value", y);
    engine.setChildProperty(mainwin, "inCursorZ_4D", "value", z);
    engine.setChildProperty(mainwin, "inCursorT_4D", "value", t);
    engine.sleep(200);
}

function openMainImage(name)
{
    //=== Opening 'Open Main' Dialog
    engine.trigger("actionOpenMain");
    engine.sleep(2000);

    //=== Entering Filename
    var dialog = engine.findChild(mainwin, "wizImageIO");
    engine.setChildProperty(dialog, "inFilename", "text", datadir + "/" + name);

    //=== Pressing the 'next' button
    engine.clickChild(dialog, "qt_wizard_commit");
    engine.sleep(1000);

    //=== Pressing the 'finish' button
    engine.clickChild(dialog, "qt_wizard_finish");
    engine.sleep(1000);
}

function openMesh(name)
{
    //=== Opening 'Open Mesh' Dialog
    engine.trigger("actionAddMesh");
    engine.sleep(2000);

    //=== Entering Filename
    var dialog = engine.findChild(mainwin, "wizMeshImport");
    engine.print("file path=" + datadir + "/" + name);
    engine.setChildProperty(dialog, "inFilename", "text", datadir + "/" + name);
    engine.sleep(1000);

    //=== Pressing the 'finish' button
    engine.clickChild(dialog, "qt_wizard_finish");
    engine.sleep(1000);

    //=== Pressing the 'OK' button
    msgbox = engine.findChild(dialog, "msgboxNewLayer");
    engine.clickChild(msgbox, "btnOK");
    engine.sleep(1000);
}

function openWorkspace(name)
{
    //=== Opening 'Open Workspace' dialog
    engine.trigger("actionOpenWorkspace");
    engine.sleep(2000);

    //=== Entering workspace filename
    var dialog = engine.findChild(mainwin, "dlgSimpleFile");
    engine.setChildProperty(dialog, "inFilename", "text", datadir + "/" + name);

    //=== Accepting text
    engine.invoke(dialog, "accept");
    engine.sleep(4000);
}
function enterSnakeMode(pos_x, pos_y, pos_z, size_x, size_y, size_z)
{
    //=== Entering snake mode
    engine.trigger("actionSnake");

    var roipanel = engine.findChild(mainwin, "pageSnakeTool");

    //=== Setting ROI position
    engine.setChildProperty(roipanel, "inIndexX", "value", pos_x);
    engine.setChildProperty(roipanel, "inIndexY", "value", pos_y);
    engine.setChildProperty(roipanel, "inIndexZ", "value", pos_z);

    //=== Setting ROI size
    engine.setChildProperty(roipanel, "inSizeX", "value", size_x);
    engine.setChildProperty(roipanel, "inSizeY", "value", size_y);
    engine.setChildProperty(roipanel, "inSizeZ", "value", size_z);

    //=== Pushing the Segment3D button
    engine.clickChild(roipanel, "btnAuto");
    engine.sleep(2000);
}
function enterSnakeModeFullROI()
{
    //=== Entering snake mode
    engine.trigger("actionSnake");

    var roipanel = engine.findChild(mainwin, "pageSnakeTool");

    //=== Resetting ROI
    engine.clickChild(roipanel, "btnResetROI");

    //=== Pushing the Segment3D button
    engine.clickChild(roipanel, "btnAuto");
    engine.sleep(2000);
}
function readVoxelIntensity(layer_row)
{
    var voxtable = engine.findChild(mainwin, "tableVoxelUnderCursor");
    value = engine.tableItemText(voxtable, layer_row, 1);

    return value;
}
function setForegroundLabel(label_text)
{
    var combo = engine.findChild(mainwin,"inForeLabel");
    var index = engine.findItemRow(combo,label_text);
    engine.print("Setting foreground label to " + label_text + " at pos " + index)
    engine.callMethod(combo, "setCurrentIndex", [index]);
}
function setBackgroundLabel(label_text)
{
    var combo = engine.findChild(mainwin,"inBackLabel");
    var index = engine.findItemRow(combo,label_text);
    engine.print("Setting background label to " + label_text + " at pos " + index)
    engine.callMethod(combo, "setCurrentIndex", [index]);
}
function resetLabels(leave_open = false)
{
    //=== Open Label Editor dialog
    engine.trigger("actionLabel_Editor");
    engine.sleep(1000);

    var dialog = engine.findChild(mainwin, "LabelEditorDialog");
    engine.trigger("actionResetLabels", dialog);

    //=== Close the dialog
    if (!leave_open)
      engine.close(dialog);
}
function openSegmentation(name)
{
    //=== Opening 'Open Segmentation' Dialog (replaces the current segmentation set)
    engine.trigger("actionLoad_from_Image");
    engine.sleep(2000);

    //=== Entering Filename
    var dialog = engine.findChild(mainwin, "wizImageIO");
    engine.setChildProperty(dialog, "inFilename", "text", datadir + "/" + name);

    //=== Pressing the 'next' button
    engine.clickChild(dialog, "qt_wizard_commit");
    engine.sleep(1000);

    //=== Pressing the 'finish' button
    engine.clickChild(dialog, "qt_wizard_finish");
    engine.sleep(2500);
}
function openAdditionalSegmentation(name)
{
    //=== Opening 'Add Segmentation > Open' Dialog
    engine.trigger("actionAddSegmentation_Open");
    engine.sleep(2000);

    //=== Entering Filename
    var dialog = engine.findChild(mainwin, "wizImageIO");
    engine.setChildProperty(dialog, "inFilename", "text", datadir + "/" + name);

    //=== Pressing the 'next' button
    engine.clickChild(dialog, "qt_wizard_commit");
    engine.sleep(1000);

    //=== Pressing the 'finish' button
    engine.clickChild(dialog, "qt_wizard_finish");
    engine.sleep(2500);
}
function loadLabelDescriptions(name)
{
    //=== Opening 'Import Label Descriptions' dialog
    engine.trigger("actionLoadLabels");
    engine.sleep(1000);

    var dialog = engine.findChild(mainwin, "dlgSimpleFile");
    engine.setChildProperty(dialog, "inFilename", "text", datadir + "/" + name);

    //=== Accepting
    engine.invoke(dialog, "accept");
    engine.sleep(500);
}
function getLayerResolutionInfo(rowObjectName)
{
    //=== Open the Layer Inspector and select the given row
    engine.trigger("actionLayerInspector");
    engine.sleep(500);

    var dlg = engine.findChild(mainwin, "dlgLayerInspector");
    var row = engine.findChild(dlg, rowObjectName);
    engine.setProperty(row, "selected", true);
    engine.sleep(500);

    //=== Switch to the Info tab
    engine.callChildMethod(dlg, "tabWidget", "setCurrentWidget", [engine.findChild(dlg, "cmpInfo")]);
    engine.sleep(500);

    var info = {
        spacingX: parseFloat(engine.getChildProperty(dlg, "outSpacingX", "text")),
        spacingY: parseFloat(engine.getChildProperty(dlg, "outSpacingY", "text")),
        spacingZ: parseFloat(engine.getChildProperty(dlg, "outSpacingZ", "text")),
        dimX: engine.getChildProperty(dlg, "outDimX", "text"),
        dimY: engine.getChildProperty(dlg, "outDimY", "text"),
        dimZ: engine.getChildProperty(dlg, "outDimZ", "text")
    };

    //=== Close the inspector dialog
    engine.invoke(dlg, "close");
    return info;
}
function updateMeshAndCheck()
{
    //=== Click the 3D pane's 'Update' button and confirm the mesh is no longer dirty
    var btn = engine.findChild(mainwin, "btnUpdateMesh");
    engine.click(btn);
    engine.sleep(3000);
    engine.validateProperty(btn, "enabled", false);
}
