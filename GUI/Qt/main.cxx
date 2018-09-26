#include <QApplication>
#include <QSettings>
#include <QAction>
#include <QShortcutEvent>
#include <QStyleFactory>
#include <QUrl>
#include <QDir>
#include <QFileSystemWatcher>

#if QT_VERSION > 0x050000
#include <QSurfaceFormat>
#endif

#include "SNAPQApplication.h"
#include "MainImageWindow.h"
#include "SliceViewPanel.h"
#include "ImageIODelegates.h"
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
#include "SNAPTestQt.h"
#include "TestOpenGLDialog.h"

#include "GenericSliceView.h"
#include "GenericSliceModel.h"
#include "GlobalUIModel.h"
#include "IRISImageData.h"

#include "itkEventObject.h"
#include "itkObject.h"
#include "itkCommand.h"
#include "vtkObject.h"

#include <iostream>
#include <clocale>
#include <cstdlib>

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



// Setting environment variables

#ifdef WIN32

template<typename TVal>
void itksnap_putenv(const std::string &var, TVal value)
{
  std::ostringstream s;
  s << var << "=" << value;
  _putenv(s.str().c_str());
}

#else

template<typename TVal>
void itksnap_putenv(const std::string &var, TVal value)
{
  std::ostringstream s;
  s << value;
  setenv(var.c_str(), s.str().c_str(), 1);
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

void usage(const char *progname)
{
  // Print usage info and exit
  cout << "ITK-SnAP Command Line Usage:" << endl;
  cout << "   " << progname << " [options] [main_image]" << endl;
  cout << "Image Options:" << endl;
  cout << "   -g FILE              : Load the main image from FILE" << endl;
  cout << "   -s FILE              : Load the segmentation image from FILE" << endl;
  cout << "   -l FILE              : Load label descriptions from FILE" << endl;
  cout << "   -o FILE [FILE+]      : Load additional images from FILE" << endl;
  cout << "                        :   (multiple files may be provided)" << endl;
  cout << "   -w FILE              : Load workspace from FILE" << endl;
  cout << "                        :   (-w cannot be mixed with -g,-s,-l,-o options)" << endl;
  cout << "Additional Options:" << endl;
  cout << "   -z FACTOR            : Specify initial zoom in screen pixels/mm" << endl;
  cout << "   --cwd PATH           : Start with PATH as the initial directory" << endl;
  cout << "   --threads N          : Limit maximum number of CPU cores used to N." << endl;
  cout << "   --scale N            : Scale all GUI elements by factor of N (e.g., 2)." << endl;
  cout << "   --geometry WxH+X+Y   : Initial geometry of the main window." << endl;
  cout << "Debugging/Testing Options:" << endl;
#ifdef SNAP_DEBUG_EVENTS
  cout << "   --debug-events       : Dump information regarding UI events" << endl;
#endif // SNAP_DEBUG_EVENTS
  cout << "   --test list          : List available tests. " << endl;
  cout << "   --test TESTID        : Execute a test. " << endl;
  cout << "   --testdir DIR        : Set the root directory for tests. " << endl;
  cout << "   --testacc factor     : Adjust the interval between test commands by factor (e.g., 0.5). " << endl;
  cout << "   --css file           : Read stylesheet from file." << endl;
  cout << "   --opengl MAJOR MINOR : Set the OpenGL major and minor version. Experimental." << endl;
  cout << "   --testgl             : Diagnose OpenGL/VTK issues." << endl;
  cout << "Platform-Specific Options:" << endl;
#if QT_VERSION < 0x050000
#ifdef Q_WS_X11
  cout << "   --x11-db             : Enable widget double buffering on X11. By default it is off." << endl;
#endif
#endif
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

  // Whether widgets are double-buffered
  bool flagX11DoubleBuffer;

  // Test-related stuff
  std::string xTestId;
  std::string fnTestDir;
  double xTestAccel;

  // Current working directory
  std::string cwd;

  // GUI related
  std::string style, cssfile;

  // OpenGL version preferred
  int opengl_major, opengl_minor;
  bool flagTestOpenGL;

  // Number of threads
  int nThreads;

  // GUI scaling
  int nDevicePixelRatio;

  // Screen geometry
  int geometry[4];

  CommandLineRequest()
    : flagDebugEvents(false), flagNoFork(false), flagConsole(false), xZoomFactor(0.0),
      flagX11DoubleBuffer(false), nThreads(0), nDevicePixelRatio(0), flagTestOpenGL(false)
    {
#if QT_VERSION >= 0x050000
    style = "fusion";
#else
    style = "plastique";
#endif
    opengl_major = 1;
    opengl_minor = 3;
    geometry[0]=geometry[1]=geometry[2]=geometry[3]=-1;
    }
};


/*
 * Define customizations to the Plastique style to make it appear more like Fusion
 */
#if QT_VERSION < 0x050000
#include <QProxyStyle>

class FusionProxy : public QProxyStyle
{
public:

  virtual void polish(QPalette &palette)
  {
    QColor fusion_gray(232, 232, 232);
    palette = QPalette(fusion_gray);
  }

protected:
};

#endif


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

  // Restrict number of threads
  // TODO: use and document this
  parser.AddOption("--threads", 1);

  // Current working directory
  parser.AddOption("--cwd", 1);

  // This dummy option is actually used internally. It's a work-around for
  // a buggy behavior on MacOS, when execvp actually causes a file
  // open event to be fired, which causes the drop dialog to open
  parser.AddOption("--dummy", 1);

  // Some qt stuff
  parser.AddOption("--style", 1);

  parser.AddOption("--x11-db",0);

  parser.AddOption("--css", 1);

  parser.AddOption("--scale", 1);

  parser.AddOption("--opengl", 2);

  parser.AddOption("--testgl", 0);

  // Standard Qt options
  parser.AddOption("--geometry", 1);
  parser.AddSynonim("--geometry", "-geometry");

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
    usage(argv[0]);
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

  // Initial directory
  if(parseResult.IsOptionPresent("--cwd"))
    argdata.cwd = parseResult.GetOptionParameter("--cwd");

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

  if(parseResult.IsOptionPresent("--css"))
    argdata.cssfile = parseResult.GetOptionParameter("--css");

  if(parseResult.IsOptionPresent("--opengl"))
    {
    argdata.opengl_major = atoi(parseResult.GetOptionParameter("--opengl", 0));
    argdata.opengl_minor = atoi(parseResult.GetOptionParameter("--opengl", 1));
    }

  if(parseResult.IsOptionPresent("--testgl"))
    argdata.flagTestOpenGL = true;


  // Enable double buffering on X11
  if(parseResult.IsOptionPresent("--x11-db"))
    argdata.flagX11DoubleBuffer = true;

  // Number of threads
  if(parseResult.IsOptionPresent("--threads"))
    argdata.nThreads = atoi(parseResult.GetOptionParameter("--threads"));

  // Number of threads
  if(parseResult.IsOptionPresent("--scale"))
    argdata.nDevicePixelRatio = atoi(parseResult.GetOptionParameter("--scale"));

  // Initial geometry
  if(parseResult.IsOptionPresent("--geometry"))
    {
    const char *geom_str = parseResult.GetOptionParameter("--geometry");
    int w, h, x = -1, y = -1;
    if(4 == sscanf(geom_str,"%dx%d+%d+%d", &w, &h, &x, &y) ||
       2 == sscanf(geom_str,"%dx%d", &w, &h))
      {
      argdata.geometry[0] = w;
      argdata.geometry[1] = h;
      argdata.geometry[2] = x;
      argdata.geometry[3] = y;
      }
    }

  return 0;
}


int test_opengl()
{
  TestOpenGLDialog *dialog = new TestOpenGLDialog();
  dialog->show();
  return QApplication::exec();
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



#if QT_VERSION > 0x050000

  // Starting with Qt 5.6, the OpenGL implementation uses OpenGL 2.0
  // In this version of OpenGL, transparency is handled differently and
  // looks wrong.
  QSurfaceFormat gl_fmt;
  gl_fmt.setMajorVersion(argdata.opengl_major);
  gl_fmt.setMinorVersion(argdata.opengl_minor);
  /*
  gl_fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
  gl_fmt.setRedBufferSize(1);
  gl_fmt.setGreenBufferSize(1);
  gl_fmt.setBlueBufferSize(1);
  gl_fmt.setDepthBufferSize(1);
  gl_fmt.setStencilBufferSize(0);
  gl_fmt.setAlphaBufferSize(0);
  */

  QSurfaceFormat::setDefaultFormat(gl_fmt);

#endif

#if QT_VERSION > 0x050400

  // Environment variable for the scale factor
  const char *QT_SCALE_FACTOR = "QT_SCALE_FACTOR";
  const char *QT_SCALE_AUTO_VAR = "QT_AUTO_SCREEN_SCALE_FACTOR";
  const char *QT_SCALE_AUTO_VALUE = "1";

#else

  const char *QT_SCALE_FACTOR = "QT_DEVICE_PIXEL_RATIO";
  const char *QT_SCALE_AUTO_VAR = "QT_DEVICE_PIXEL_RATIO";
  const char *QT_SCALE_AUTO_VALUE = "auto";

#endif

  /* -----------------------------
   * DEAL WITH PIXEL RATIO SCALING
   * ----------------------------- */

  // Read the pixel ratio from environment or command line
  int devicePixelRatio = 0;
  if(argdata.nDevicePixelRatio > 0)
    devicePixelRatio = argdata.nDevicePixelRatio;
  else if(getenv("ITKSNAP_SCALE_FACTOR"))
    devicePixelRatio = atoi(getenv("ITKSNAP_SCALE_FACTOR"));

  // Set the environment variable
  if(devicePixelRatio > 0)
    {
    itksnap_putenv(QT_SCALE_FACTOR,devicePixelRatio);
    }
  else
    {
    itksnap_putenv(QT_SCALE_AUTO_VAR, QT_SCALE_AUTO_VALUE);
    }

  // Turn off event debugging if needed
#ifdef SNAP_DEBUG_EVENTS
  flag_snap_debug_events = argdata.flagDebugEvents;
#endif

  // Setup crash signal handlers
  SetupSignalHandlers();

  // Deal with threads
  if(argdata.nThreads > 0)
    itk::MultiThreader::SetGlobalMaximumNumberOfThreads(argdata.nThreads);

  // Turn off ITK and VTK warning windows
  itk::Object::GlobalWarningDisplayOff();
  vtkObject::GlobalWarningDisplayOff();

  // Connect Qt to the Renderer subsystem
  AbstractRenderer::SetPlatformSupport(new QtRendererPlatformSupport());

  // Create an application
  SNAPQApplication app(argc, argv);
  Q_INIT_RESOURCE(SNAPResources);
  Q_INIT_RESOURCE(TestingScripts);


  // Reset the locale to posix to avoid weird issues with NRRD files
  std::setlocale(LC_NUMERIC, "POSIX");

  // Force use of native OpenGL, since all of our functions and VTK use native
  // and cannot use ANGLE
  // TODO: we haven't proven that this actually helps with anything so hold off..
  // app.setAttribute(Qt::AA_UseDesktopOpenGL);


  // Set the application style
  app.setStyle(QStyleFactory::create(argdata.style.c_str()));
  if(argdata.style != "fusion")
    {
    QPalette fpal(QColor(232,232,232));
    fpal.setColor(QPalette::Normal, QPalette::Highlight, QColor(70, 136, 228));
    app.setPalette(fpal);
    }

  // Test OpenGL?
  if(argdata.flagTestOpenGL)
    {
    return test_opengl();
    }

  // Before we can create any of the framework classes, we need to get some
  // platform-specific functionality to the SystemInterface
  QtSystemInfoDelegate siDelegate;
  SystemInterface::SetSystemInfoDelegate(&siDelegate);

  // Create the global UI
  try 
    {
    SmartPtr<GlobalUIModel> gui = GlobalUIModel::New();
    IRISApplication *driver = gui->GetDriver();

    // Set the initial directory. The fallthough is to set to the user's home
    // directory
    QString init_dir = QDir::homePath();
    QString app_dir = QApplication::applicationDirPath();

    // Also get the directory one up from the application dir (this is because
    // on windows "run in" defaults to one up dir)
    QDir app_up_qdir(app_dir); app_up_qdir.cdUp();
    QString app_up_dir = app_up_qdir.path();

    // If the user provides a flag for the current directory, try using it but
    // only if this is a valid directory
    if(argdata.cwd.size())
      {
      QDir dir(from_utf8(argdata.cwd));
      if(dir.exists() && dir.isReadable())
        {
        init_dir = dir.absolutePath();
        }
      }
    else if(QDir::currentPath().length() > 1 &&
            QDir::currentPath() != app_dir &&
            QDir::currentPath() != app_up_dir)
      {
      init_dir = QDir::currentPath();
      }

    gui->GetGlobalState()->SetInitialDirectory(to_utf8(init_dir));

    // Load the user preferences
    gui->LoadUserPreferences();

    // Create the main window
    MainImageWindow *mainwin = new MainImageWindow();
    mainwin->Initialize(gui);

    // Load stylesheet
    if(argdata.cssfile.size())
      {
      QFileSystemWatcher *watcher = new QFileSystemWatcher(mainwin);
      watcher->addPath(from_utf8(argdata.cssfile));
      QObject::connect(watcher, SIGNAL(fileChanged(QString)),
              mainwin, SLOT(externalStyleSheetFileChanged(QString)));
      }

    // Disable double buffering in X11 to avoid flickering issues. The documentation
    // says this only happens on X11. For the time being, we are only implementing this
    // for Qt4 and X11
#if QT_VERSION < 0x050000
#ifdef Q_WS_X11
    if(!argdata.flagX11DoubleBuffer)
      mainwin->setAttribute(Qt::WA_PaintOnScreen);
#endif
#endif

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

    // Set the initial dimensions of the main window if asked for
    if(argdata.geometry[0] > 0 && argdata.geometry[1] > 0)
      {
      mainwin->resize(QSize(argdata.geometry[0], argdata.geometry[1]));
      if(argdata.geometry[2] >= 0 && argdata.geometry[3] >= 0)
        {
        mainwin->move(argdata.geometry[2], argdata.geometry[3]);
        }
      }

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

