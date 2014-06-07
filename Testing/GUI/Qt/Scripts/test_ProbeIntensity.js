// This script positions the cursor and checks image intensity

//--- source OpenImage
engine.print("Setting cursor position")
var cursx = engine.findChild(mainwin, "inCursorX")
var cursy = engine.findChild(mainwin, "inCursorY")
var cursz = engine.findChild(mainwin, "inCursorZ")
cursx.value = 75;
cursy.value = 6;
cursz.value = 27;

//--- sleep 600
engine.print("Pulling up image information")
var table = engine.findChild(mainwin, "tableVoxelUnderCursor")
var value = engine.tableItemText(table, 0, 1)
engine.print("Result is " + value)

//--- sleep 600
engine.validateValue(value, 54)
