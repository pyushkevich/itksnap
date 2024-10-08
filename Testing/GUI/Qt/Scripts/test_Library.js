function setCursor(x, y, z)
{
  engine.print("Setting cursor position to " + x + ", " + y + ", " + z);
  engine.findChild(mainwin, "inCursorX").value = x;
  engine.findChild(mainwin, "inCursorY").value = y;
  engine.findChild(mainwin, "inCursorZ").value = z;
  engine.sleep(200);
}
function setCursor4D(x, y, z, t)
{
    engine.print("Setting cursor position to " + x + ", " + y + ", " + z + ", " + t);
    engine.findChild(mainwin, "inCursorX_4D").value = x;
    engine.findChild(mainwin, "inCursorY_4D").value = y;
    engine.findChild(mainwin, "inCursorZ_4D").value = z;
    engine.findChild(mainwin, "inCursorT_4D").value = t;
    engine.sleep(200);
}

function openMainImage(name)
{
    //=== Opening 'Open Main' Dialog
    engine.trigger("actionOpenMain");
    engine.sleep(2000);

    //=== Entering Filename
    var dialog = engine.findChild(mainwin, "wizImageIO");
    engine.findChild(dialog, "inFilename").text = datadir + "/" + name;

    //=== Pressing the 'next' button
    engine.findChild(dialog, "qt_wizard_commit").click();
    engine.sleep(1000);

    //=== Pressing the 'finish' button
    engine.findChild(dialog, "qt_wizard_finish").click();
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
    engine.findChild(dialog, "inFilename").text = datadir + "/" + name;
    engine.sleep(1000);

    //=== Pressing the 'finish' button
    engine.findChild(dialog, "qt_wizard_finish").click();
    engine.sleep(1000);

    //=== Pressing the 'OK' button
    msgbox = engine.findChild(dialog, "msgboxNewLayer");
    engine.findChild(msgbox, "btnOK").click();
    engine.sleep(1000);
}

function openWorkspace(name)
{
    //=== Opening 'Open Workspace' dialog
    engine.trigger("actionOpenWorkspace");
    engine.sleep(2000);

    //=== Entering workspace filename
    var dialog = engine.findChild(mainwin, "dlgSimpleFile");
    engine.findChild(dialog, "inFilename").text = datadir + "/" + name;

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
    engine.findChild(roipanel,"inIndexX").value = pos_x;
    engine.findChild(roipanel,"inIndexY").value = pos_y;
    engine.findChild(roipanel,"inIndexZ").value = pos_z;

    //=== Setting ROI size
    engine.findChild(roipanel,"inSizeX").value = size_x;
    engine.findChild(roipanel,"inSizeY").value = size_y;
    engine.findChild(roipanel,"inSizeZ").value = size_z;

    //=== Pushing the Segment3D button
    engine.findChild(roipanel,"btnAuto").click();
    engine.sleep(2000);
}
function enterSnakeModeFullROI()
{
    //=== Entering snake mode
    engine.trigger("actionSnake");

    var roipanel = engine.findChild(mainwin, "pageSnakeTool");

    //=== Resetting ROI
    engine.findChild(roipanel,"btnResetROI").click();

    //=== Pushing the Segment3D button
    engine.findChild(roipanel,"btnAuto").click();
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
    combo.setCurrentIndex(index);
}
function setBackgroundLabel(label_text)
{
    var combo = engine.findChild(mainwin,"inBackLabel");
    var index = engine.findItemRow(combo,label_text);
    engine.print("Setting background label to " + label_text + " at pos " + index)
    combo.setCurrentIndex(index);
}
