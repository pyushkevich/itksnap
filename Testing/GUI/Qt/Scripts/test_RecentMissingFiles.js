// Read the function library
include("Library");

// Recent files that no longer exist are marked on the startup page (#314).
// SNAP_TEST_TMPDIR is a scratch folder set up by CMakeLists.txt.
var tmpdir = engine.getEnv("SNAP_TEST_TMPDIR");
var copy = tmpdir + "/RecentMissingFiles.gipl.gz";
engine.validateValue(engine.copyFile(datadir + "/MRIcrop-orig.gipl.gz", copy), true);

// Open an image that stays, then the copy, so that both are in the history
// with the copy first
openMainImage("MRIcrop-orig.gipl.gz");
waitForMainImage();
engine.sleep(1000);
engine.trigger("actionOpenMain");
engine.sleep(2000);
var dialog = engine.findChild(mainwin, "wizImageIO");
engine.setChildProperty(dialog, "inFilename", "text", copy);
engine.clickChild(dialog, "qt_wizard_commit");
engine.sleep(1000);
engine.clickChild(dialog, "qt_wizard_finish");
engine.sleep(1000);
waitForMainImage();

// Delete the copy and go back to the startup page. Wait past the interval at
// which files are checked again.
engine.sleep(3000);
engine.validateValue(engine.removeFile(copy), true);
engine.trigger("actionUnload_All");
engine.sleep(3000);

var panel = engine.findChild(mainwin, "panelRecentImages");
var list = engine.findChild(panel, "listRecent");
var MissingFileRole = 256 + 1;     // Qt::UserRole + 1
var TooltipRole = 3;               // Qt::ToolTipRole

//=== The copy is marked missing, the image that exists is not
engine.validateValue(String(engine.itemData(list, 0, 0, 256)), copy);
engine.validateValue(engine.itemData(list, 0, 0, MissingFileRole), true);
var tooltip = String(engine.itemData(list, 0, 0, TooltipRole));
engine.print("Tooltip: " + tooltip);
engine.validateValue(tooltip.indexOf("not found") >= 0, true);
engine.validateValue(String(engine.itemData(list, 1, 0, 256)).endsWith("MRIcrop-orig.gipl.gz"), true);
engine.validateValue(engine.itemData(list, 1, 0, MissingFileRole), false);

// Find the action for a file in a menu (or a button's menu)
function findAction(menu, file)
{
    var actions = engine.menuActions(menu);
    for (var i = 0; i < actions.length; i++)
        if (String(actions[i].data) == file)
            return actions[i];
    for (var i = 0; i < actions.length; i++)
        engine.print("  " + actions[i].text);
    engine.testFailed("No menu entry for " + file);
}

//=== File > Recent Images: the copy is disabled and marked, the other is not
var menu = engine.findChild(mainwin, "menuRecent_Images");
var action = findAction(menu, copy);
engine.print("Recent menu: " + action.text);
engine.validateValue(action.enabled, false);
engine.validateValue(String(action.text).endsWith("(not found)"), true);
action = findAction(menu, datadir + "/MRIcrop-orig.gipl.gz");
engine.validateValue(action.enabled, true);
engine.validateValue(String(action.text), datadir + "/MRIcrop-orig.gipl.gz");

//=== The history drop-down of the Open Main Image dialog marks it too
engine.trigger("actionOpenMain");
engine.sleep(2000);
dialog = engine.findChild(mainwin, "wizImageIO");
action = findAction(engine.findChild(dialog, "btnHistory"), copy);
engine.print("History drop-down: " + action.text);
engine.validateValue(action.enabled, false);
engine.validateValue(String(action.text).endsWith("(not found)"), true);
engine.invoke(dialog, "reject");
engine.sleep(1000);

//=== Clicking it offers to remove it; Cancel keeps it
var n = engine.itemCount(list);
engine.clickItem(list, 0);
engine.sleep(1000);
var mbox = engine.findWidget("mboxRecentFileMissing");
if (!mbox || !engine.getProperty(mbox, "visible"))
    engine.testFailed("Clicking a missing recent file did not ask what to do");
engine.clickChild(mbox, "btnCancel");
engine.sleep(1000);
engine.validateValue(engine.itemCount(list), n);

//=== Remove from List removes it
engine.clickItem(list, 0);
engine.sleep(1000);
mbox = engine.findWidget("mboxRecentFileMissing");
if (!mbox || !engine.getProperty(mbox, "visible"))
    engine.testFailed("Clicking a missing recent file did not ask what to do");
engine.clickChild(mbox, "btnRemove");
engine.sleep(1500);
engine.validateValue(engine.itemCount(list), n - 1);
engine.validateValue(String(engine.itemData(list, 0, 0, 256)).endsWith("MRIcrop-orig.gipl.gz"), true);
