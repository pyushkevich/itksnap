// Hello world!

// --- sleep 600
engine.print("Opening Dialog")
var myaction = engine.findChild(mainwin,"actionOpenMain");
myaction.trigger()

// --- sleep 600
engine.print("Entering Text")
var dialog = engine.findChild(mainwin, "wizImageIO");
var textfield = engine.findChild(dialog, "inFilename");
textfield.text = datadir;

// --- sleep 600
engine.print("Pressing the 'next' button")
var button = engine.findChild(dialog, "qt_wizard_commit")
button.click()

// --- sleep 2000
engine.print("Pressing the 'finish' button")
var button = engine.findChild(dialog, "qt_wizard_finish")
button.click()

// --- sleep 2000
engine.print("Setting cursor position")
var cursx = engine.findChild(mainwin, "inCursorX")
var cursy = engine.findChild(mainwin, "inCursorY")
var cursz = engine.findChild(mainwin, "inCursorZ")
cursx.value = 75;
cursy.value = 6;
cursz.value = 27;

// --- sleep 600
engine.print("Pulling up image information")
var table = engine.findChild(mainwin, "tableVoxelUnderCursor")
var value = engine.tableItemText(table, 0, 1)
engine.print("Result is " + value)

// --- sleep 600
engine.validateValue(value, 54)
