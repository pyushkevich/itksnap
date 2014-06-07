// Hello world!

//--- sleep 600
engine.print("Opening Dialog")
var myaction = engine.findChild(mainwin,"actionOpenMain");
myaction.trigger()

//--- sleep 600
engine.print("Entering Text")
var dialog = engine.findChild(mainwin, "wizImageIO");
var textfield = engine.findChild(dialog, "inFilename");
textfield.text = datadir + "/MRIcrop-orig.gipl.gz";

//--- sleep 600
engine.print("Pressing the 'next' button")
var button = engine.findChild(dialog, "qt_wizard_commit")
button.click()

//--- sleep 2000
engine.print("Pressing the 'finish' button")
var button = engine.findChild(dialog, "qt_wizard_finish")
button.click()

//--- sleep 2000
