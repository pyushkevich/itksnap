#include <QApplication>
#include <QSettings>
#include "MainImageWindow.h"
#include "SliceViewPanel.h"
#include "IRISApplication.h"
#include "SNAPAppearanceSettings.h"

#include "GenericSliceView.h"
#include "GenericSliceModel.h"
#include "GlobalUIModel.h"
#include "IRISImageData.h"

#include "itkEventObject.h"
#include "itkObject.h"
#include "itkCommand.h"

#include <QPlastiqueStyle>

#include "IRISMainToolbox.h"

int main(int argc, char *argv[])
{
  // Create an application
  QApplication app(argc, argv);
  Q_INIT_RESOURCE(SNAPResources);

  app.setStyle(new QPlastiqueStyle);
  /*
  QPalette qp = app.palette();
  qp.setColor(QPalette::Button, QColor(120,160,240));
  app.setPalette(qp);
  */

  // Create the global UI
  SmartPtr<GlobalUIModel> gui = GlobalUIModel::New();
  IRISApplication *driver = gui->GetDriver();

  // Load the user preferences
  driver->GetSystemInterface()->LoadUserPreferences();

  // Create the main window
  MainImageWindow mainwin;
  mainwin.SetModel(gui);

  // Set up the dock widget
  QDockWidget *dock = new QDockWidget("IRIS Toolbox", &mainwin);
  IRISMainToolbox *tbx = new IRISMainToolbox(&mainwin);
  tbx->SetModel(gui);
  dock->setWidget(tbx);
  mainwin.addDockWidget(Qt::LeftDockWidgetArea, dock);

  // Do some assembly here (yuck)
  for(int i = 0; i < 3; i++)
    {
    mainwin.GetSlicePanel(i)->Initialize(gui, i);
    }

  // Read an image
  if(argc > 1)
    {
    driver->LoadMainImage(argv[1],IRISApplication::MAIN_ANY);
    if(argc > 2)
      {
      //gui.GetDriver()->LoadOverlayImage(argv[2], IRISApplication::MAIN_ANY);
      if(argc > 3)
        {
        driver->LoadLabelImageFile(argv[3]);
        }
      }
    }

  // Show the panel
  mainwin.show();

  // Run application
  int rc = app.exec();

  // If everything cool, save the preferences
  if(!rc)
    driver->GetSystemInterface()->SaveUserPreferences();
}

