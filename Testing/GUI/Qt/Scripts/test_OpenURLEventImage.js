// Read the function library
include("Library");

// ITK-SNAP is started with --test-url, which opens an itksnap-http:// URL
// with a query string and a fragment the way macOS opens a browser link: as
// a file open event. The test server (serve_slowly.py --require-query, see
// CMakeLists.txt) refuses the download unless the query arrives unchanged.
// The macOS handler used to drop the query and fragment.
waitForMainImage(30);
engine.sleep(1000);
engine.print("Image opened from the URL with its query string");
