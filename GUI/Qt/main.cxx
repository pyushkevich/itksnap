#include <QApplication>
#include <QSettings>
#include "MainImageWindow.h"
#include "SliceViewPanel.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "SNAPAppearanceSettings.h"
#include "CommandLineArgumentParser.h"
#include "SliceWindowCoordinator.h"
#include "SnakeWizardPanel.h"
#include "QtScriptTest1.h"
#include "QtRendererPlatformSupport.h"
#include "QtIPCManager.h"
#include "QtCursorOverride.h"
#include "SNAPQtCommon.h"

#include "GenericSliceView.h"
#include "GenericSliceModel.h"
#include "GlobalUIModel.h"
#include "IRISImageData.h"

#include "itkEventObject.h"
#include "itkObject.h"
#include "itkCommand.h"
#include "vtkObject.h"

#include <QPlastiqueStyle>
#include <QWindowsVistaStyle>
#include <QAction>

#include "ImageIODelegates.h"

#include <iostream>
#include "SNAPTestQt.h"

using namespace std;

// Setup printing of stack trace on segmentation faults. This only
// works on POSIX systems
#if defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))

#include <signal.h>
#include <execinfo.h>

void SegmentationFaultHandler(int sig)
{
  cerr << "*************************************" << endl;
  cerr << "ITK-SNAP: " << sys_siglist[sig] << endl;
  cerr << "BACKTRACE: " << endl;
  void *array[50];
  int nsize = backtrace(array, 50);
  backtrace_symbols_fd(array, nsize, 2);
  cerr << "*************************************" << endl;
  exit(-1);
}

void SetupSignalHandlers()
{
  signal(SIGSEGV, SegmentationFaultHandler);
}

#else

void SetupSignalHandlers()
{
  // Nothing to do!
}

#endif

/** Class to handle exceptions in Qt callbacks */
class SNAPQApplication : public QApplication
{
public:
  SNAPQApplication(int argc, char **argv) :
    QApplication(argc, argv)
  {
    this->setApplicationName("ITK-SNAP");
    this->setOrganizationName("itksnap.org");
  }

  bool notify(QObject *object, QEvent *event)
  {
    try { return QApplication::notify(object, event); }
    catch(std::exception &exc)
    {
      // Crash!
      ReportNonLethalException(NULL, exc, "Unexpected Error",
                               "ITK-SNAP has crashed due to an unexpected error");

      // Exit the application
      QApplication::exit(-1);

      return false;
    }
  }
};


#ifdef SNAP_DEBUG_EVENTS
bool flag_snap_debug_events = false;
#endif

void usage()
{
  // Print usage info and exit
  cout << "ITK-SnAP Command Line Usage:" << endl;
  cout << "   snap [options] [main_image]" << endl;
  cout << "Image Options:" << endl;
  cout << "   -g FILE              : Load the greyscale image from FILE" << endl;
  cout << "   -s FILE              : Load the segmentation image from FILE" << endl;
  cout << "   -l FILE              : Load label descriptions from FILE" << endl;
  cout << "   -o FILE              : Load overlay image from FILE" << endl;
  cout << "                        :   (-o option can be repeated multiple times)" << endl;
  cout << "   -w FILE              : Load workspace from FILE" << endl;
  cout << "                        :   (-w cannot be mixed with -g,-s,-l,-o options)" << endl;
  cout << "Additional Options:" << endl;
  cout << "   -z FACTOR            : Specify initial zoom in screen pixels/mm" << endl;
  cout << "Debugging/Testing Options" << endl;
#ifdef SNAP_DEBUG_EVENTS
  cout << "   --debug-events       : Dump information regarding UI events" << endl;
#endif // SNAP_DEBUG_EVENTS
  cout << "   --test list          : List available tests. " << endl;
  cout << "   --test TESTID        : Execute a test. " << endl;
  cout << "   --testdir DIR        : Set the root directory for tests. " << endl;
  cout << "   --testQtScript index : Runs QtScript based test indexed by index. " << endl;
}

void setupParser(CommandLineArgumentParser &parser)
{
  // Parse command line parameters
  parser.AddOption("--grey",1);
  parser.AddSynonim("--grey","-g");

  parser.AddOption("--segmentation",1);
  parser.AddSynonim("--segmentation","-s");
  parser.AddSynonim("--segmentation","-seg");

  parser.AddOption("--overlay", -1);
  parser.AddSynonim("--overlay", "-o");

  parser.AddOption("--labels",1);
  parser.AddSynonim("--labels","--label");
  parser.AddSynonim("--labels","-l");

  parser.AddOption("--workspace", 1);
  parser.AddSynonim("--workspace", "-w");

  parser.AddOption("--zoom", 1);
  parser.AddSynonim("--zoom", "-z");

  parser.AddOption("--compact", 1);
  parser.AddSynonim("--compact", "-c");

  parser.AddOption("--help", 0);
  parser.AddSynonim("--help", "-h");

  parser.AddOption("--debug-events", 0);

  parser.AddOption("--test", 1);
  parser.AddOption("--testdir", 1);

  parser.AddOption("--testQtScript", 1);
}

#include <QScriptEngine>
#include <QScriptEngineDebugger>

void scriptChildren(QScriptEngine &engine, QObject *widget, QString parent)
{
  for(int i = 0; i < widget->children().size(); i++)
    {
    QObject *c = widget->children()[i];

    if(dynamic_cast<QWidget *>(c) || dynamic_cast<QAction *>(c))
      {
      QString name = QString("%1_%2").arg(parent).arg(c->objectName());
      QScriptValue val = engine.newQObject(c);
      engine.globalObject().setProperty(name, val);
      scriptChildren(engine, c, name);
      }

    }
}


int main(int argc, char *argv[])
{
  // Setup crash signal handlers
  SetupSignalHandlers();

  // Turn off ITK and VTK warning windows
  itk::Object::GlobalWarningDisplayOff();
  vtkObject::GlobalWarningDisplayOff();


  // Connect Qt to the Renderer subsystem
  AbstractRenderer::SetPlatformSupport(new QtRendererPlatformSupport());

  // Parse command line arguments
  CommandLineArgumentParser parser;
  CommandLineArgumentParseResult parseResult;
  int iTrailing = 0;

  setupParser(parser);
  if(!parser.TryParseCommandLine(argc,argv,parseResult,false,iTrailing))
    {
    cerr << "Unable to parse command line. Run " << argv[0] << " -h for help" << endl;
    return -1;
    }

  // Need help?
  if(parseResult.IsOptionPresent("--help"))
    {
    usage();
    return 0;
    }

  // Parse this option before anything else!
  if(parseResult.IsOptionPresent("--debug-events"))
    {
#ifdef SNAP_DEBUG_EVENTS
    flag_snap_debug_events = true;
#else
    cerr << "Option --debug-events ignored because ITK-SNAP was compiled "
            "without the SNAP_DEBUG_EVENTS option. Please recompile." << endl;
#endif
    }

  // Create an application
  SNAPQApplication app(argc, argv);
  Q_INIT_RESOURCE(SNAPResources);

  app.setStyle(new QPlastiqueStyle);

  // Before we can create any of the framework classes, we need to get some
  // platform-specific functionality to the SystemInterface
  QtSystemInfoDelegate siDelegate;
  SystemInterface::SetSystemInfoDelegate(&siDelegate);

  // Create the global UI
  SmartPtr<GlobalUIModel> gui;
  IRISApplication *driver;
  try
  {
    gui = GlobalUIModel::New();
    driver = gui->GetDriver();

    // Load the user preferences
    gui->LoadUserPreferences();
  }
  catch(itk::ExceptionObject &exc)
  {
    ReportNonLethalException(NULL, exc, "ITK-SNAP failed to start", "Exception occurred during ITK-SNAP startup");
    exit(-1);
  }

  // Create the main window
  MainImageWindow *mainwin = new MainImageWindow();
  mainwin->Initialize(gui);

  // We let the main window handle events to the application
  app.installEventFilter(mainwin);

  // Start parsing options
  const char *fnMain = NULL, *fnWorkspace = NULL;
  IRISWarningList warnings;

  // Check if a workspace is being loaded
  if(parseResult.IsOptionPresent("--workspace"))
    {
    // Put a waiting cursor
    QtCursorOverride curse(Qt::WaitCursor);

    // Check for incompatible options
    if(parseResult.IsOptionPresent("--grey")
       || parseResult.IsOptionPresent("--overlay")
       || parseResult.IsOptionPresent("--labels")
       || parseResult.IsOptionPresent("--segmentation"))
      {
      cerr << "Error: Option -w may not be used with -g, -o, -l or -s options." << endl;
      return -1;
      }

    // Get the workspace filename
    fnWorkspace = parseResult.GetOptionParameter("--workspace");

    // Load the workspace
    try
      {
      driver->OpenProject(fnWorkspace, warnings);
      }
    catch(itk::ExceptionObject &exc)
      {
      cerr << "Error loading workspace '" << fnWorkspace << "'" << endl;
      cerr << "Reason: " << exc << endl;
      return -1;
      }
    }

  // No workspace, just images
  else
    {
    // The following situations are possible for main image
    // itksnap file                       <- load as main image, detect file type
    // itksnap --gray file                <- load as main image, force gray
    // itksnap --gray file1 file2         <- ignore file2

    // Check if a main image file is specified
    if(parseResult.IsOptionPresent("--grey"))
      {
      fnMain = parseResult.GetOptionParameter("--grey");
      }
    else if(iTrailing < argc)
      {
      fnMain = argv[iTrailing];
      }

    // If no main, there should be no overlays, segmentation
    if(!fnMain && parseResult.IsOptionPresent("--segmentation"))
      {
      cerr << "Error: Option -s must be used together with option -g" << endl;
      return -1;
      }

    if(!fnMain && parseResult.IsOptionPresent("--overlay"))
      {
      cerr << "Error: Option -p must be used together with option -g" << endl;
      return -1;
      }

    // Load main image file
    if(fnMain)
      {
      // Put a waiting cursor
      QtCursorOverride curse(Qt::WaitCursor);

      // Update the splash screen
      // ui->UpdateSplashScreen("Loading image...");

      // Try loading the image
      try
        {
        driver->LoadImage(fnMain, MAIN_ROLE, warnings);
        }
      catch(itk::ExceptionObject &exc)
        {
        cerr << "Error loading image '" << fnMain << "'" << endl;
        cerr << "Reason: " << exc << endl;
        return -1;
        }

      // Load the segmentation if supplied
      if(parseResult.IsOptionPresent("--segmentation"))
        {
        // Get the filename
        const char *fname = parseResult.GetOptionParameter("--segmentation");

        // Update the splash screen
        // ui->UpdateSplashScreen("Loading segmentation image...");

        // Try to load the image
        try
          {
          driver->LoadImage(fname, LABEL_ROLE, warnings);
          }
        catch(itk::ExceptionObject &exc)
          {
          cerr << "Error loading segmentation '" << fname << "'" << endl;
          cerr << "Reason: " << exc << endl;
          return -1;
          }
        }

      // Load overlay fs supplied
      if(parseResult.IsOptionPresent("--overlay"))
        {
        for(int i = 0; i < parseResult.GetNumberOfOptionParameters("--overlay"); i++)
          {
          // Get the filename
          const char *fname = parseResult.GetOptionParameter("--overlay", i);

          // Update the splash screen
          // ui->UpdateSplashScreen("Loading overlay image...");

          // Try to load the image
          try
            {
            driver->LoadImage(fname, OVERLAY_ROLE, warnings);
            }
          catch(std::exception &exc)
            {
            cerr << "Error loading overlay '" << fname << "'" << endl;
            cerr << "Reason: " << exc.what() << endl;
            return -1;
            }
          }
        }
      } // if main image filename supplied

    // Load labels if supplied
    if(parseResult.IsOptionPresent("--labels"))
      {
      // Get the filename
      const char *fname = parseResult.GetOptionParameter("--labels");

      // Update the splash screen
      // ui->UpdateSplashScreen("Loading label descriptions...");

      try
        {
        // Load the label file
        driver->LoadLabelDescriptions(fname);
        }
      catch(itk::ExceptionObject &exc)
        {
        cerr << "Error reading label descriptions: " <<
          exc.GetDescription() << endl;
        }
      }
    } // Not loading workspace

  // Set initial zoom if specified
  if(parseResult.IsOptionPresent("--zoom"))
    {
    double zoom = atof(parseResult.GetOptionParameter("--zoom"));
    if(zoom >= 0.0)
      {
      gui->GetSliceCoordinator()->SetLinkedZoom(true);
      gui->GetSliceCoordinator()->SetZoomLevelAllWindows(zoom);
      }
    else
      {
      cerr << "Invalid zoom level (" << zoom << ") specified" << endl;
      }
    }

  /*

  if(parseResult.IsOptionPresent("--compact"))
    {
    string slice = parseResult.GetOptionParameter("--compact");
    if(slice.length() == 0 || !(slice[0] == 'a' || slice[0] == 'c' || slice[0] == 's'))
      cerr << "Wrong parameter passed for '--compact', ignoring" << endl;
    else
      {
      DisplayLayout dl = ui->GetDisplayLayout();
      dl.show_main_ui = false;
      ui->SetDisplayLayout(dl);
      dl.show_panel_ui = false;
      ui->SetDisplayLayout(dl);
      dl.size = HALF_SIZE;
      ui->SetDisplayLayout(dl);
      dl.slice_config = slice[0] == 'a' ? AXIAL : (slice[0] == 'c' ? CORONAL : SAGITTAL);
      ui->SetDisplayLayout(dl);
      }
    }
    */

  // Play with scripting
  QScriptEngine engine;
  QScriptEngineDebugger bugger;
  bugger.attachTo(&engine);

  // Find all the child widgets of mainwin
  engine.globalObject().setProperty("snap", engine.newQObject(mainwin));

  // Configure the IPC communications (as a hidden widget)
  QtIPCManager *ipcman = new QtIPCManager(mainwin);
  ipcman->hide();
  ipcman->SetModel(gui->GetSynchronizationModel());

  // Start in cross-hairs mode
  gui->GetGlobalState()->SetToolbarMode(CROSSHAIRS_MODE);

  // Show the panel
  mainwin->ShowFirstTime();

  if(parseResult.IsOptionPresent("--test"))
    {
    std::string root = ".";
    if(parseResult.IsOptionPresent("--testdir"))
      root = parseResult.GetOptionParameter("--testdir");

    SNAPTestQt tester;
    tester.Initialize(mainwin, gui, root);
    tester.RunTest(parseResult.GetOptionParameter("--test"));
    }

  if(parseResult.IsOptionPresent("--testQtScript"))
    {
    int nIndxTest = atoi(parseResult.GetOptionParameter("--testQtScript"));
    cout << "Prototype Test with QtScript executed - Test nr " << nIndxTest << endl;
    cout << "CheckResultQtScript" << endl;

    // Should have one script C++ class, multiple text scripts in QtScript format

    //QtScriptTest1(&eng  ine);
    //Yes, with memory leak so far
    QtScriptTest1 * pTest1 = new QtScriptTest1();
    pTest1->Initialize(mainwin, gui, "");
    pTest1->Run(&engine);

    //return(0);
    }

  // Check for updates?
  mainwin->UpdateAutoCheck();

  // Run application
  int rc = app.exec();

  // If everything cool, save the preferences
  if(!rc)
    gui->SaveUserPreferences();

  // Unload the main image before all the destructors start firing
  driver->UnloadMainImage();

  // Get rid of the main window while the model is still alive
  delete mainwin;

  // Destroy the model after the GUI is destroyed
  gui = NULL;
}

