#include <string>

#include "qapplication.h"

#include "OrientationWidgetGUI.h"

using namespace std;

int main(int argc, char** argv)
{

	QApplication app(argc, argv);

    OrientationWidgetGUI owGUI;

    owGUI.show();

	int nRetCode = app.exec();

	return(nRetCode);

}

