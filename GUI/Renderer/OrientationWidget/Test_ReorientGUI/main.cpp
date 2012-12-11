#include <string>

#include "qapplication.h"

#include "ReorientGUI.h"

using namespace std;

int main(int argc, char** argv)
{

	QApplication app(argc, argv);

	ReorientGUI reorientGUI;

	reorientGUI.show();

	int nRetCode = app.exec();

	return(nRetCode);

}

