function setCursor(x, y, z)
{
  engine.print("Setting cursor position to " + x + ", " + y + ", " + z);
  engine.findChild(mainwin, "inCursorX").value = x;
  engine.findChild(mainwin, "inCursorY").value = y;
  engine.findChild(mainwin, "inCursorZ").value = z;
}

function openMainImage(name)
{
  //=== Opening Dialog
  engine.findChild(mainwin,"actionOpenMain").trigger();

  //=== Entering Text
  var dialog = engine.findChild(mainwin, "wizImageIO");
  engine.findChild(dialog, "inFilename").text = datadir + "/" + name;

  //=== Pressing the 'next' button
  engine.findChild(dialog, "qt_wizard_commit").click();
  thread.wait(1000);

  //=== Pressing the 'finish' button
  engine.findChild(dialog, "qt_wizard_finish").click();
  thread.wait(1000);
}

function enterSnakeMode(pos_x, pos_y, pos_z, size_x, size_y, size_z)
{
  //=== Entering snake mode
  engine.findChild(mainwin,"actionSnake").trigger();

  var roipanel = engine.findChild(mainwin, "pageSnakeTool");

  //=== Setting ROI dimensions
  engine.findChild(roipanel,"inIndexX").value = pos_x;
  engine.findChild(roipanel,"inIndexY").value = pos_y;
  engine.findChild(roipanel,"inIndexZ").value = pos_z;
  engine.findChild(roipanel,"inSizeX").value = size_x;
  engine.findChild(roipanel,"inSizeY").value = size_y;
  engine.findChild(roipanel,"inSizeZ").value = size_z;

  //=== Pushing the Segment3D button
  engine.findChild(roipanel,"btnAuto").click();
  thread.wait(1000);
}

function readVoxelIntensity(layer_row)
{
  var voxtable = engine.findChild(mainwin, "tableVoxelUnderCursor");
  value = engine.tableItemText(voxtable, layer_row, 1);

  return value;
}
