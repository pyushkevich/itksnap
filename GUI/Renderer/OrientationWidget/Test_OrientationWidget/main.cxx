#include <string>

#include "qapplication.h"

#include "OrientationWidgetGUI.h"

#ifdef SNAP_DEBUG_EVENTS
bool flag_snap_debug_events = false;
#endif

using namespace std;

int main(int argc, char** argv)
{

	QApplication app(argc, argv);

    OrientationWidgetGUI owGUI;

    owGUI.show();

	int nRetCode = app.exec();

	return(nRetCode);

}

