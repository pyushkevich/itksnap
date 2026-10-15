// Read the function library
include("Library");

// The Getting Started page links to sample workspaces with itksnap-https://
// URLs. Clicking one must open the workspace in this window instead of
// sending the link to the web browser. serve_slowly.py (see CMakeLists.txt)
// serves the test data on 127.0.0.1:8921, so that no network is needed.
var browser = engine.findChild(mainwin, "brsGettingStarted");

//=== The page links to the five sample workspaces
var html = String(engine.getProperty(browser, "html"));
var base = "itksnap-https://huggingface.co/datasets/upennpatchlab/itksnap_sample_data/resolve/main/";
var n_links = html.split(base).length - 1;
engine.print("Sample dataset links on the Getting Started page: " + n_links);
engine.validateValue(n_links, 5);

//=== Clicking a link opens the workspace in this window
engine.callMethod(browser, "anchorClicked",
                  ["itksnap-http://127.0.0.1:8921/MRIcrop-with-mesh.itksnap"]);
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
engine.print("Layers after clicking the link: " + n);
engine.validateValue(n > 1, true);
