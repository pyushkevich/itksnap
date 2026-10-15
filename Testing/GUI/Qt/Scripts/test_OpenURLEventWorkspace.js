// Read the function library
include("Library");

// ITK-SNAP is started with --test-url, which opens an itksnap-http:// link to
// a workspace, with a query string, as macOS opens a browser link (see
// CMakeLists.txt). The URL must be recognized as a workspace even though it
// does not end in .itksnap; otherwise it is loaded as an image, which fails.
waitForMainImage(30);
engine.sleep(1000);

// The workspace has a mesh layer, which an image would not bring
engine.trigger("actionLayerInspector");
engine.sleep(1000);
var inspector = engine.findChild(mainwin, "dlgLayerInspector");
var n = 0;
while (engine.findChild(inspector, "wgtRowDelegate_000" + n) != null)
    n++;
engine.close(inspector);
engine.print("Layers after opening the workspace URL: " + n);
engine.validateValue(n > 1, true);
