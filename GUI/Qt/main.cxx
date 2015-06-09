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

#include <QStyleFactory>
#include <QAction>
#include <QUrl>

#include "ImageIODelegates.h"

#include <iostream>
#include "SNAPTestQt.h"

using namespace std;

// Interrupt handler. This will attempt to clean up

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


/*
 * Code to handle forking the application on startup. Forking is desirable
 * because many users execute SNAP from command line, and it is annoying to
 * have SNAP blocking the command line. Also, this reduced the frequency
 * of users interrupting SNAP or killing it by closing the terminal.
 */

#include <QFileOpenEvent>
#include <QTime>
#include <QMessageBox>

/** Class to handle exceptions in Qt callbacks */
class SNAPQApplication : public QApplication
{
public:
  SNAPQApplication(int &argc, char **argv) :
    QApplication(argc, argv)
    {
    this->setApplicationName("ITK-SNAP");
    this->setOrganizationName("itksnap.org");
    this->setAttribute(Qt::AA_UseHighDpiPixmaps, true);
    m_MainWindow = NULL;

    // Store the command-line arguments
    for(int i = 1; i < argc; i++)
      m_Args.push_back(QString::fromUtf8(argv[i]));
  }

  void setMainWindow(MainImageWindow *mainwin)
  {
    m_MainWindow = mainwin;
    m_StartupTime = QTime::currentTime();
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


  virtual bool event(QEvent *event)
  {
    if (event->type() == QEvent::FileOpen && m_MainWindow)
      {
      QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(event);
      QString file = openEvent->url().path();

      // MacOS bug - we get these open document events automatically generated
      // from command-line parameters, and I have no idea why. To avoid this,
      // if the event occurs at startup (within a second), we will check if
      // the passed in URL matches the command-line arguments, and ignore it
      // if it does
      if(m_StartupTime.secsTo(QTime::currentTime()) < 1)
        {
        foreach(const QString &arg, m_Args)
          {
          if(arg == file)
            return true;
          }
        }

      // Ok, we passed the check, now it's safe to actually open the file
      m_MainWindow->raise();
      m_MainWindow->LoadDroppedFile(file);
      return true;
      }
    else return QApplication::event(event);
  }

private:
  MainImageWindow *m_MainWindow;
  QStringList m_Args;
  QTime m_StartupTime;
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
  cout << "   --testacc factor     : Adjust the interval between test commands by factor (e.g., 0.5). " << endl;
}

void setupParser(CommandLineArgumentParser &parser)
{
}

/**
 * This class describes the command-line options parsed from the command line.
 */
struct CommandLineRequest
{
public:
  std::string fnMain;
  std::vector<std::string> fnOverlay;
  std::string fnSegmentation;
  std::string fnLabelDesc;
  std::string fnWorkspace;
  double xZoomFactor;
  bool flagDebugEvents;

  // Whether the console-based application should not fork
  bool flagNoFork;

  // Whether the application is being launched from the console
  bool flagConsole;

  // Test-related stuff
  std::string xTestId;
  std::string fnTestDir;
  double xTestAccel;

  // GUI related
  std::string style;

  CommandLineRequest()
    : flagDebugEvents(false), flagNoFork(false), flagConsole(false), xZoomFactor(0.0) 
    {
    style = "fusion";
    }
};

/**
 This function decodes filenames in "SHORT" DOS format. It does nothing
 on non-Windows platforms
*/
std::string DecodeFilename(const std::string &in_string)
{
#ifdef WIN32
  int bufsize = GetLongPathName(in_string.c_str(), NULL, 0);
  if (bufsize == 0)
    throw IRISException("Unable to decode parameter %s", in_string.c_str());

  char *buffer = new char[bufsize];
  int rc = GetLongPathName(in_string.c_str(), buffer, bufsize);
  
  if (rc == 0)
    throw IRISException("Unable to decode parameter %s", in_string.c_str());

  return std::string(buffer);

#else
  return in_string;

#endif

}


/**
 * This function takes the command-line arguments and parses them into the
 * CommandLineRequest structure. If it returns with a non-zero error code,
 * the program should exit with that code.
 */
int parse(int argc, char *argv[], CommandLineRequest &argdata)
{

  // Parse command line arguments
  CommandLineArgumentParser parser;

  // These are all the recognized arguments
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

  // TO BE ADDED
  // parser.AddOption("--compact", 1);
  // parser.AddSynonim("--compact", "-c");

  parser.AddOption("--help", 0);
  parser.AddSynonim("--help", "-h");

  parser.AddOption("--debug-events", 0);

  parser.AddOption("--no-fork", 0);
  parser.AddOption("--console", 0);

  parser.AddOption("--test", 1);
  parser.AddOption("--testdir", 1);
  parser.AddOption("--testacc", 1);

  // This dummy option is actually used internally. It's a work-around for
  // a buggy behavior on MacOS, when execvp actually causes a file
  // open event to be fired, which causes the drop dialog to open
  parser.AddOption("--dummy", 1);

  // Some qt stuff
  parser.AddOption("--style", 1);

  // Obtain the result
  CommandLineArgumentParseResult parseResult;

  // Number of trailing arguments
  int iTrailing = 0;

  // Set up the command line parser with all the options
  if(!parser.TryParseCommandLine(argc, argv, parseResult, false, iTrailing))
    {
    cerr << "Unable to parse command line. Run " << argv[0] << " -h for help" << endl;
    return -1;
    }

  // Need help?
  if(parseResult.IsOptionPresent("--help"))
    {
    usage();
    return 1;
    }

  // Parse this option before anything else!
  if(parseResult.IsOptionPresent("--debug-events"))
    {
#ifdef SNAP_DEBUG_EVENTS
    argdata.flagDebugEvents = true;
#else
    cerr << "Option --debug-events ignored because ITK-SNAP was compiled "
            "without the SNAP_DEBUG_EVENTS option. Please recompile." << endl;
#endif
    }

  // Check if a workspace is being loaded
  if(parseResult.IsOptionPresent("--workspace"))
    {
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
    argdata.fnWorkspace = DecodeFilename(parseResult.GetOptionParameter("--workspace"));
    }

  // No workspace, just images
  else
    {
    // The following situations are possible for main image
    // itksnap file                       <- load as main image, detect file type
    // itksnap --gray file                <- load as main image, force gray
    // itksnap --gray file1 file2         <- ignore file2

    // Check if a main image file is specified
    bool have_main = false;
    if(parseResult.IsOptionPresent("--grey"))
      {
      argdata.fnMain = DecodeFilename(parseResult.GetOptionParameter("--grey"));
      have_main = true;
      }
    else if(iTrailing < argc)
      {
      argdata.fnMain = DecodeFilename(argv[iTrailing]);
      have_main = true;
      }

    // If no main, there should be no overlays, segmentation
    if(!have_main && parseResult.IsOptionPresent("--segmentation"))
      {
      cerr << "Error: Option -s must be used together with option -g" << endl;
      return -1;
      }

    if(!have_main && parseResult.IsOptionPresent("--overlay"))
      {
      cerr << "Error: Option -p must be used together with option -g" << endl;
      return -1;
      }

    // Load main image file
    if(have_main)
      {
      // Load the segmentation if supplied
      if(parseResult.IsOptionPresent("--segmentation"))
        {
        // Get the filename
        argdata.fnSegmentation = DecodeFilename(parseResult.GetOptionParameter("--segmentation"));
        }

      // Load overlay fs supplied
      if(parseResult.IsOptionPresent("--overlay"))
        {
        for(int i = 0; i < parseResult.GetNumberOfOptionParameters("--overlay"); i++)
          {
          // Get the filename
          argdata.fnOverlay.push_back(DecodeFilename(parseResult.GetOptionParameter("--overlay", i)));
          }
        }
      } // if main image filename supplied

    // Load labels if supplied
    if(parseResult.IsOptionPresent("--labels"))
      {
      // Get the filename
      argdata.fnLabelDesc = DecodeFilename(parseResult.GetOptionParameter("--labels"));
      }
    } // Not loading workspace

  // Set initial zoom if specified
  if(parseResult.IsOptionPresent("--zoom"))
    {
    argdata.xZoomFactor = atof(parseResult.GetOptionParameter("--zoom"));
    if(argdata.xZoomFactor <= 0.0)
      {
      cerr << "Invalid zoom level (" << argdata.xZoomFactor << ") specified" << endl;
      }
    }

  // Console flag. On non-Apple UNIX, the console flag is ignored and default
  // behavior is to fork. On Win/Apple, default behavior is not to fork (assume
  // launch from OS GUI, ability to respond to open document event on Apple
#if defined(__UNIX__) && !defined(__APPLE__)
  argdata.flagConsole = true;
#else
  argdata.flagConsole = parseResult.IsOptionPresent("--console");
#endif

  // Forking behavior.
  argdata.flagNoFork = parseResult.IsOptionPresent("--no-fork");

  // Testing
  if(parseResult.IsOptionPresent("--test"))
    {
    argdata.xTestId = parseResult.GetOptionParameter("--test");
    if(parseResult.IsOptionPresent("--testdir"))
      argdata.fnTestDir = DecodeFilename(parseResult.GetOptionParameter("--testdir"));
    else
      argdata.fnTestDir = ".";

    if(parseResult.IsOptionPresent("--testacc"))
      argdata.xTestAccel = atof(parseResult.GetOptionParameter("--testacc"));
    else
      argdata.xTestAccel = 1.0;

    }

  // GUI stuff
  if(parseResult.IsOptionPresent("--style"))
    argdata.style = parseResult.GetOptionParameter("--style");

  return 0;
}


int main(int argc, char *argv[])
{  
  // Test object, which only is allocated if tests are requested. The
  // reason we declare it here is that the test object allocates a
  // script engine, which must be deleted at the very end
  SNAPTestQt *testingEngine = NULL;

  // Parse the command line
  CommandLineRequest argdata;
  int exitcode = parse(argc, argv, argdata);
  if(exitcode != 0)
    return exitcode;

  // If the program is executed from the console, we would like it to
  // background and outlive the console. At this point, we can ditch the
  // connection with the parent shell, i.e., fork the program.
  if(argdata.flagConsole && !argdata.flagNoFork)
    SystemInterface::LaunchChildSNAP(argc, argv, true);

  // Debugging mechanism: if no-fork is on, sleep for 60 secs
  // if(argdata.flagNoFork)
  //  sleep(60);

  // Turn off event debugging if needed
#ifdef SNAP_DEBUG_EVENTS
  flag_snap_debug_events = argdata.flagDebugEvents;
#endif

  // Setup crash signal handlers
  SetupSignalHandlers();

  // Turn off ITK and VTK warning windows
  itk::Object::GlobalWarningDisplayOff();
  vtkObject::GlobalWarningDisplayOff();

  // Connect Qt to the Renderer subsystem
  AbstractRenderer::SetPlatformSupport(new QtRendererPlatformSupport());

  // Create an application
  SNAPQApplication app(argc, argv);
  Q_INIT_RESOURCE(SNAPResources);
  Q_INIT_RESOURCE(TestingScripts);

  // Set the application style
  app.setStyle(QStyleFactory::create(argdata.style.c_str()));

  // Before we can create any of the framework classes, we need to get some
  // platform-specific functionality to the SystemInterface
  QtSystemInfoDelegate siDelegate;
  SystemInterface::SetSystemInfoDelegate(&siDelegate);

  // Create the global UI
  try 
    {
    SmartPtr<GlobalUIModel> gui = GlobalUIModel::New();
    IRISApplication *driver = gui->GetDriver();

    // Load the user preferences
    gui->LoadUserPreferences();

    // Create the main window
    MainImageWindow *mainwin = new MainImageWindow();
    mainwin->Initialize(gui);

    // Start parsing options
    IRISWarningList warnings;

    // Check if a workspace is being loaded
    if(argdata.fnWorkspace.size())
      {
      // Put a waiting cursor
      QtCursorOverride curse(Qt::WaitCursor);

      // Load the workspace
      try
        {
        driver->OpenProject(argdata.fnWorkspace, warnings);
        }
      catch(std::exception &exc)
        {
        ReportNonLethalException(mainwin, exc, "Workspace Error",
                                 QString("Failed to load workspace %1").arg(
                                   from_utf8(argdata.fnWorkspace)));
        }
      }
    else
      {
      // Load main image file
      if(argdata.fnMain.size())
        {
        // Put a waiting cursor
        QtCursorOverride curse(Qt::WaitCursor);

        // Try loading the image
        try
          {
          // Load the main image. If that fails, all else should fail too
          driver->LoadImage(argdata.fnMain.c_str(), MAIN_ROLE, warnings);

          // Load the segmentation
          if(argdata.fnSegmentation.size())
            {
            try
              {
              driver->LoadImage(argdata.fnSegmentation.c_str(), LABEL_ROLE, warnings);
              }
            catch(std::exception &exc)
              {
              ReportNonLethalException(mainwin, exc, "Image IO Error",
                                       QString("Failed to load segmentation %1").arg(
                                         from_utf8(argdata.fnSegmentation)));
              }
            }

          // Load the overlays
          if(argdata.fnOverlay.size())
            {
            std::string current_overlay;
            try
            {
              for(int i = 0; i < argdata.fnOverlay.size(); i++)
                {
                current_overlay = argdata.fnOverlay[i];
                driver->LoadImage(current_overlay.c_str(), OVERLAY_ROLE, warnings);
                }
            }
            catch(std::exception &exc)
              {
              ReportNonLethalException(mainwin, exc, "Overlay IO Error",
                                       QString("Failed to load overlay %1").arg(
                                         from_utf8(current_overlay)));
              }
            }
          }
        catch(std::exception &exc)
          {
          ReportNonLethalException(mainwin, exc, "Image IO Error",
                                   QString("Failed to load image %1").arg(
                                     from_utf8(argdata.fnMain)));
          }
        } // if main image filename supplied

      if(argdata.fnLabelDesc.size())
        {
        try
          {
          // Load the label file
          driver->LoadLabelDescriptions(argdata.fnLabelDesc.c_str());
          }
        catch(std::exception &exc)
          {
          ReportNonLethalException(mainwin, exc, "Label Description IO Error",
                                   QString("Failed to load labels from %1").arg(
                                     from_utf8(argdata.fnLabelDesc)));
          }
        }
      } // Not loading workspace

    // Zoom level
    if(argdata.xZoomFactor > 0)
      {
      gui->GetSliceCoordinator()->SetLinkedZoom(true);
      gui->GetSliceCoordinator()->SetZoomLevelAllWindows(argdata.xZoomFactor);
      }

    /*
     * ADD THIS LATER!

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

    // Configure the IPC communications (as a hidden widget)
    QtIPCManager *ipcman = new QtIPCManager(mainwin);
    ipcman->hide();
    ipcman->SetModel(gui->GetSynchronizationModel());

    // Start in cross-hairs mode
    gui->GetGlobalState()->SetToolbarMode(CROSSHAIRS_MODE);

    // Show the panel
    mainwin->ShowFirstTime();

    // Check for updates?
    mainwin->UpdateAutoCheck();

    // Assign the main window to the application. We do this right before
    // starting the event loop.
    app.setMainWindow(mainwin);

    // Do the test
    if(argdata.xTestId.size())
      {
      testingEngine = new SNAPTestQt(mainwin, argdata.fnTestDir, argdata.xTestAccel);
      testingEngine->LaunchTest(argdata.xTestId);
      }

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

    // Destory the test engine
    if(testingEngine)
      delete testingEngine;

    // Exit with the return code
    std::cerr << "Return code : " << rc << std::endl;
    return rc;
    }
  catch(std::exception &exc)
    {
    ReportNonLethalException(NULL, exc, "ITK-SNAP failed to start", "Exception occurred during ITK-SNAP startup");
    exit(-1);
    }

}

