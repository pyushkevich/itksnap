#include "IRISApplication.h"
#include "MeshImportModel.h"
#include "QtLocalDeepLearningServerDelegate.h"
#include "RESTClient.h"
#include "SNAPQApplication.h"
#include "MainImageWindow.h"
#include "ImageIODelegates.h"
#include "CommandLineArgumentParser.h"
#include "SliceWindowCoordinator.h"
#include "SnakeWizardPanel.h"
#include "QtRendererPlatformSupport.h"
#include "QtIPCManager.h"
#include "QtCursorOverride.h"
#include "QtReporterDelegates.h"
#include "SNAPQtCommon.h"
#include "SNAPTestQt.h"

#include "GenericSliceModel.h"
#include "SynchronizationModel.h"
#include "GlobalUIModel.h"

#include "itkObject.h"
#include "vtkObject.h"

#include <QtCore/qtranslator.h>
#include <iostream>
#include <clocale>
#include <cstdlib>

#include <QApplication>
#include <QSettings>
#include <QAction>
#include <QShortcutEvent>
#include <QStyleFactory>
#include <QUrl>
#include <QDir>
#include <QFileSystemWatcher>

#if QT_VERSION > 0x050000
#  include <QSurfaceFormat>
#endif


#include "QVTKOpenGLNativeWidget.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkTextActor.h"
#include <vtkLogger.h>
#include <QHBoxLayout>
#include <QPushButton>
#include <QStandardPaths>
#include <QMessageBox>
#include <QDesktopServices>
#include "IRISImageData.h"


using namespace std;

// Interrupt handler. This will attempt to clean up

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))

#include <execinfo.h>

void printBacktrace()
{
  cerr << "*************************************" << endl;
  cerr << "BACKTRACE: " << endl;
  void *array[50];
  int   nsize = backtrace(array, 50);
  backtrace_symbols_fd(array, nsize, 2);
  cerr << "*************************************" << endl;

}

#else

#  include <windows.h>
#  include <dbghelp.h>
#  pragma comment(lib, "dbghelp.lib")

void
printBacktrace()
{
  std::cerr << "*************************************" << std::endl;
  std::cerr << "BACKTRACE: " << std::endl;

  // Initialize symbol handler
  HANDLE process = GetCurrentProcess();
  SymInitialize(process, nullptr, TRUE);

  // Capture the backtrace
  void  *stack[50];
  USHORT frames = CaptureStackBackTrace(0, 50, stack, nullptr);

  // Symbol info structure
  SYMBOL_INFO *symbol = (SYMBOL_INFO *)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
  symbol->MaxNameLen = 255;
  symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

  for (USHORT i = 0; i < frames; ++i)
  {
    DWORD64 address = (DWORD64)(stack[i]);
    if (SymFromAddr(process, address, 0, symbol))
    {
      std::cerr << i << ": " << symbol->Name << " - 0x" << std::hex << symbol->Address << std::endl;
    }
    else
    {
      std::cerr << i << ": [Unable to retrieve symbol]" << std::endl;
    }
  }

  free(symbol);
  SymCleanup(process);

  std::cerr << "*************************************" << std::endl;
}


#endif



QString
BackupSegmentationToEmergencyFile()
{
  try
  {
    SNAPQApplication *app = dynamic_cast<SNAPQApplication *>(QCoreApplication::instance());
    MainImageWindow  *mwin = app ? app->mainWindow() : nullptr;
    IRISApplication  *driver = mwin ? mwin->GetModel()->GetDriver() : nullptr;
    std::list<ImageWrapperBase *> unsaved;
    if (driver)
    {
      for (LayerIterator it = driver->GetIRISImageData()->GetLayers(LABEL_ROLE); !it.IsAtEnd(); ++it)
      {
        if (it.GetLayer()->HasUnsavedChanges())
          unsaved.push_back(it.GetLayer());
      }
    }

    if (unsaved.size())
    {
      // Get a system-appropriate directory for cache or application data
      QString backupDirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
      if (backupDirPath.isEmpty())
      {
        qWarning() << "Failed to determine a writable location for application data.";
        return QString();
      }

      // Append application name to create a specific directory
      QDir backupDir(backupDirPath);
      if (!backupDir.mkpath("CrashRecovery"))
      {
        qWarning() << "Failed to create backup directory:" << backupDirPath;
        return QString();
      }

      backupDir.cd("CrashRecovery");

      // Create a unique backup file name with timestamp
      QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
      QString backupDirName = QString("Recovery_%1").arg(timestamp);
      backupDir.mkpath(backupDirName);
      backupDir.cd(backupDirName);

      // Loop over the segmentation files and save them
      GenericImageData     *id = driver->GetIRISImageData();
      std::set<std::string> saved_files;
      unsigned int          layer_number = 0;
      for (auto *layer : unsaved)
      {
        char buffer[256];
        snprintf(buffer, 256, "segmentation_backup_layer_%03d.nii.gz", ++layer_number);
        QString  fn = QString::fromUtf8(buffer);
        QString  backupFilePath = backupDir.filePath(fn);
        Registry dummy;
        layer->WriteToFile(backupFilePath.toUtf8(), dummy);
      }

      // Show a message dialog
      qCritical() << "Crash recovery backup files written to: " << backupDir.absolutePath();
      return backupDir.absolutePath();
    }
  }
  catch (...)
  {
    qCritical() << "Exception raised trying to save emergency backup";
  }

  return QString();
}

void
test_terminate_handler()
{
  // Save backup file
  QString backup_dir = BackupSegmentationToEmergencyFile();

  // Detach synchronization - otherwise a stale state is left
  SNAPQApplication *app = dynamic_cast<SNAPQApplication *>(QCoreApplication::instance());
  MainImageWindow  *mwin = app ? app->mainWindow() : nullptr;
  GlobalUIModel *model = mwin ? mwin->GetModel() : nullptr;
  if(model)
    model->GetSynchronizationModel()->ForceDetach();

  // Print stack trace
  printBacktrace();

  // Let the user know what we did
  if (backup_dir.size())
  {
    try
    {
      // Create the critical message box
      QMessageBox msgBox;
      msgBox.setIcon(QMessageBox::Critical);
      msgBox.setText(QString("ITK-SNAP crashed due to an unexpected error. Your unsaved "
                             "segmentations have been saved to folder '%1'")
                       .arg(backup_dir));
      msgBox.setWindowTitle(QObject::tr("Crash Recovery"));

      // Add the "Open Folder" button
      QPushButton *openFolderButton = msgBox.addButton(QObject::tr("Open Folder"), QMessageBox::AcceptRole);

      // Add the "Close" button
      msgBox.addButton(QObject::tr("Close"), QMessageBox::RejectRole);

      // Execute the message box and handle the button press
      msgBox.exec();

      // Check if the "Open Folder" button was clicked
      if (msgBox.clickedButton() == openFolderButton)
      {
        // Open the folder in the system's file manager
        QUrl folderUrl = QUrl::fromLocalFile(backup_dir);
        if (!QDesktopServices::openUrl(folderUrl))
        {
          QMessageBox::warning(nullptr, QObject::tr("Error"), QObject::tr("Failed to open the folder: ") + backup_dir);
        }
      }
    }
    catch (...)
    {}
  }

  // Terminate program
  std::abort();
}


#ifdef WIN32

template <typename TVal>
void
itksnap_putenv(const std::string &var, TVal value)
{
  std::ostringstream s;
  s << var << "=" << value;
  _putenv(s.str().c_str());
}

#else

template <typename TVal>
void
itksnap_putenv(const std::string &var, TVal value)
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

void
usage(const char *progname)
{
  // Print usage info and exit
  cout << "ITK-SnAP Command Line Usage:" << endl;
  cout << "   " << progname << " [options] [main_image]" << endl;
  cout << "Image Options:" << endl;
  cout << "   -g FILE              : Load the main image from FILE" << endl;
  cout << "   -s FILE [FILE+]      : Load the segmentation image from FILE" << endl;
  cout << "                        :   (multiple space separated files may be provided)" << endl;
  cout << "   -l FILE              : Load label descriptions from FILE" << endl;
  cout << "   -o FILE [FILE+]      : Load additional images from FILE" << endl;
  cout << "                        :   (multiple space separated files may be provided)" << endl;
  cout << "   -m FILE [FILE+]      : Load additional meshes from FILE" << endl;
  cout << "                        :   (multiple space separated files may be provided)" << endl;
  cout << "   -w FILE              : Load workspace from FILE" << endl;
  cout << "                        :   (-w cannot be mixed with -g,-s,-l,-o options)" << endl;
  cout << "Additional Options:" << endl;
  cout << "   -z FACTOR            : Specify initial zoom in screen pixels/mm" << endl;
  cout << "   --cwd PATH           : Start with PATH as the initial directory" << endl;
  cout << "   --threads N          : Limit maximum number of CPU cores used to N." << endl;
  cout << "   --scale N            : Scale all GUI elements by factor of N (e.g., 2)." << endl;
  cout << "   --geometry WxH+X+Y   : Initial geometry of the main window." << endl;
  cout << "   --lang <code>        : Set the language for the GUI to ISO 639-1 language code like 'es' or 'cn'" << endl;
  cout << "Debugging/Testing Options:" << endl;
#ifdef SNAP_DEBUG_EVENTS
  cout << "   --debug-events       : Dump information regarding UI events" << endl;
#endif // SNAP_DEBUG_EVENTS
  cout << "   --test list          : List available tests. " << endl;
  cout << "   --test TESTID        : Execute a test. " << endl;
  cout << "   --testdir DIR        : Set the root directory for tests. " << endl;
  cout << "   --testacc factor     : Adjust the interval between test commands by factor (e.g., "
          "0.5). "
       << endl;
  cout << "   --css file           : Read stylesheet from file." << endl;
  cout << "   --opengl MAJOR MINOR : Set the OpenGL major and minor version. Experimental." << endl;
  cout << "   --testgl             : Diagnose OpenGL/VTK issues." << endl;
  cout << "Platform-Specific Options:" << endl;
#if QT_VERSION < 0x050000
#  ifdef Q_WS_X11
  cout << "   --x11-db             : Enable widget double buffering on X11. By default it is off."
       << endl;
#  endif
#endif
}

void
setupParser(CommandLineArgumentParser &parser)
{}

/**
 * This class describes the command-line options parsed from the command line.
 */
struct CommandLineRequest
{
public:
  std::string              fnMain;
  std::vector<std::string> fnOverlay;
  std::vector<std::string> fnSegmentation;
  std::vector<std::string> fnMesh;
  std::string              fnLabelDesc;
  std::string              fnWorkspace;
  double                   xZoomFactor = 0.0;
  bool                     flagDebugEvents = false;
  bool                     flagDebugSync = false;

  // Whether the console-based application should not fork
  bool flagNoFork = false;

  // Whether the application is being launched from the console
  bool flagConsole = false;

  // Whether widgets are double-buffered
  bool flagX11DoubleBuffer = false;

  // Test-related stuff
  std::string xTestId;
  std::string fnTestDir;
  double      xTestAccel = 1.0;

  // Current working directory
  std::string cwd;

  // GUI related
  std::string style = "fusion", cssfile;

  // OpenGL version preferred
  int  opengl_major = 1, opengl_minor = 3;
  bool flagTestOpenGL = false;

  // Number of threads
  int nThreads = 0;

  // GUI scaling
  int nDevicePixelRatio = 0;

  // Screen geometry
  int geometry[4] = {-1, -1, -1, -1};

  // language code or tag
  std::string gui_language;

};


/*
 * Define customizations to the Plastique style to make it appear more like Fusion
 */
#if QT_VERSION < 0x050000
#  include <QProxyStyle>

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
std::string
DecodeFilename(const std::string &in_string)
{
#ifdef WIN32
  int bufsize = GetLongPathNameA(in_string.c_str(), NULL, 0);
  if (bufsize == 0)
    throw IRISException("Unable to decode parameter %s", in_string.c_str());

  char *buffer = new char[bufsize];
  int   rc = GetLongPathNameA(in_string.c_str(), buffer, bufsize);

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
int
parse(int argc, char *argv[], CommandLineRequest &argdata)
{

  // Parse command line arguments
  CommandLineArgumentParser parser;

  // These are all the recognized arguments
  parser.AddOption("--grey", 1);
  parser.AddSynonim("--grey", "-g");

  parser.AddOption("--segmentation", -1);
  parser.AddSynonim("--segmentation", "-s");
  parser.AddSynonim("--segmentation", "-seg");

  parser.AddOption("--overlay", -1);
  parser.AddSynonim("--overlay", "-o");

  parser.AddOption("--mesh", -1);
  parser.AddSynonim("--mesh", "-m");

  parser.AddOption("--labels", 1);
  parser.AddSynonim("--labels", "--label");
  parser.AddSynonim("--labels", "-l");

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
  parser.AddOption("--debug-sync", 0);

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

  parser.AddOption("--x11-db", 0);

  parser.AddOption("--css", 1);

  parser.AddOption("--scale", 1);

  parser.AddOption("--opengl", 2);

  parser.AddOption("--testgl", 0);

  // Standard Qt options
  parser.AddOption("--geometry", 1);
  parser.AddSynonim("--geometry", "-geometry");

  // Translation
  parser.AddOption("--lang", 1);

  // Obtain the result
  CommandLineArgumentParseResult parseResult;

  // Number of trailing arguments
  int iTrailing = 0;

  // Set up the command line parser with all the options
  if (!parser.TryParseCommandLine(argc, argv, parseResult, false, iTrailing))
  {
    cerr << "Unable to parse command line. Run " << argv[0] << " -h for help" << endl;
    return -1;
  }

  // Need help?
  if (parseResult.IsOptionPresent("--help"))
  {
    usage(argv[0]);
    return 1;
  }

  // Parse this option before anything else!
  if (parseResult.IsOptionPresent("--debug-events"))
  {
#ifdef SNAP_DEBUG_EVENTS
    argdata.flagDebugEvents = true;
#else
    cerr << "Option --debug-events ignored because ITK-SNAP was compiled "
            "without the SNAP_DEBUG_EVENTS option. Please recompile."
         << endl;
#endif
  }

  if (parseResult.IsOptionPresent("--debug-sync"))
    argdata.flagDebugSync = true;

  // Initial directory
  if (parseResult.IsOptionPresent("--cwd"))
    argdata.cwd = parseResult.GetOptionParameter("--cwd");

  // Check if a workspace is being loaded
  if (parseResult.IsOptionPresent("--workspace"))
  {
    // Check for incompatible options
    if (parseResult.IsOptionPresent("--grey") || parseResult.IsOptionPresent("--overlay") ||
        parseResult.IsOptionPresent("--labels") || parseResult.IsOptionPresent("--segmentation") ||
        parseResult.IsOptionPresent("--mesh"))
    {
      cerr << "Error: Option -w may not be used with -g, -o, -l, -m or -s options." << endl;
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
    if (parseResult.IsOptionPresent("--grey"))
    {
      argdata.fnMain = DecodeFilename(parseResult.GetOptionParameter("--grey"));
      have_main = true;
    }
    else if (iTrailing < argc)
    {
      argdata.fnMain = DecodeFilename(argv[iTrailing]);
      have_main = true;
    }

    // If no main, there should be no overlays, segmentation
    if (!have_main && parseResult.IsOptionPresent("--segmentation"))
    {
      cerr << "Error: Option -s must be used together with option -g" << endl;
      return -1;
    }

    if (!have_main && parseResult.IsOptionPresent("--overlay"))
    {
      cerr << "Error: Option -o must be used together with option -g" << endl;
      return -1;
    }

    if (!have_main && parseResult.IsOptionPresent("--mesh"))
    {
      cerr << "Error: Option -m must be used together with option -g" << endl;
      return -1;
    }

    // Load main image file
    if (have_main)
    {
      // Load the segmentation if supplied
      if (parseResult.IsOptionPresent("--segmentation"))
      {
        // Get the filename
        for (int i = 0; i < parseResult.GetNumberOfOptionParameters("--segmentation"); i++)
        {
          argdata.fnSegmentation.push_back(
            DecodeFilename(parseResult.GetOptionParameter("--segmentation", i)));
        }
      }

      // Load overlay if supplied
      if (parseResult.IsOptionPresent("--overlay"))
      {
        for (int i = 0; i < parseResult.GetNumberOfOptionParameters("--overlay"); i++)
        {
          // Get the filename
          argdata.fnOverlay.push_back(DecodeFilename(parseResult.GetOptionParameter("--overlay", i)));
        }
      }

      // Load meshes if supplied
      if (parseResult.IsOptionPresent("--mesh"))
      {
        for (int i = 0; i < parseResult.GetNumberOfOptionParameters("--mesh"); i++)
        {
          // Get the filename
          argdata.fnMesh.push_back(DecodeFilename(parseResult.GetOptionParameter("--mesh", i)));
        }
      }
    } // if main image filename supplied

    // Load labels if supplied
    if (parseResult.IsOptionPresent("--labels"))
    {
      // Get the filename
      argdata.fnLabelDesc = DecodeFilename(parseResult.GetOptionParameter("--labels"));
    }
  } // Not loading workspace

  // Set initial zoom if specified
  if (parseResult.IsOptionPresent("--zoom"))
  {
    argdata.xZoomFactor = atof(parseResult.GetOptionParameter("--zoom"));
    if (argdata.xZoomFactor <= 0.0)
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
  if (parseResult.IsOptionPresent("--test"))
  {
    argdata.xTestId = parseResult.GetOptionParameter("--test");
    if (parseResult.IsOptionPresent("--testdir"))
      argdata.fnTestDir = DecodeFilename(parseResult.GetOptionParameter("--testdir"));
    else
      argdata.fnTestDir = ".";

    if (parseResult.IsOptionPresent("--testacc"))
      argdata.xTestAccel = atof(parseResult.GetOptionParameter("--testacc"));
    else
      argdata.xTestAccel = 1.0;
  }

  // GUI stuff
  if (parseResult.IsOptionPresent("--style"))
    argdata.style = parseResult.GetOptionParameter("--style");

  if (parseResult.IsOptionPresent("--css"))
    argdata.cssfile = parseResult.GetOptionParameter("--css");

  if (parseResult.IsOptionPresent("--opengl"))
  {
    argdata.opengl_major = atoi(parseResult.GetOptionParameter("--opengl", 0));
    argdata.opengl_minor = atoi(parseResult.GetOptionParameter("--opengl", 1));
  }

  if (parseResult.IsOptionPresent("--testgl"))
    argdata.flagTestOpenGL = true;


  // Enable double buffering on X11
  if (parseResult.IsOptionPresent("--x11-db"))
    argdata.flagX11DoubleBuffer = true;

  // Number of threads
  if (parseResult.IsOptionPresent("--threads"))
    argdata.nThreads = atoi(parseResult.GetOptionParameter("--threads"));

  // Number of threads
  if (parseResult.IsOptionPresent("--scale"))
    argdata.nDevicePixelRatio = atoi(parseResult.GetOptionParameter("--scale"));

  // Initial geometry
  if (parseResult.IsOptionPresent("--geometry"))
  {
    const char *geom_str = parseResult.GetOptionParameter("--geometry");
    int         w, h, x = -1, y = -1;
    if (4 == sscanf(geom_str, "%dx%d+%d+%d", &w, &h, &x, &y) || 2 == sscanf(geom_str, "%dx%d", &w, &h))
    {
      argdata.geometry[0] = w;
      argdata.geometry[1] = h;
      argdata.geometry[2] = x;
      argdata.geometry[3] = y;
    }
  }

  // Locale
  if(parseResult.IsOptionPresent("--lang"))
  {
    argdata.gui_language = parseResult.GetOptionParameter("--lang");
  }

  return 0;
}

int
main(int argc, char *argv[])
{
  // Set locale to UTF8 on Windows, this allows files with non-ANSI characters to be loaded
#ifdef WIN32
  std::setlocale(LC_ALL, ".UTF8");
#endif

  // Test object, which only is allocated if tests are requested. The
  // reason we declare it here is that the test object allocates a
  // script engine, which must be deleted at the very end
  SNAPTestQt *testingEngine = NULL;

  std::cout << "Launching ITK-SNAP" << std::endl;

  // Parse the command line
  CommandLineRequest argdata;
  int                exitcode = parse(argc, argv, argdata);
  if (exitcode != 0)
    return exitcode;

  // If the program is executed from the console, we would like it to
  // background and outlive the console. At this point, we can ditch the
  // connection with the parent shell, i.e., fork the program.
  if (argdata.flagConsole && !argdata.flagNoFork)
    SystemInterface::LaunchChildSNAP(argc, argv, true);

  // Debugging mechanism: if no-fork is on, sleep for 60 secs
  // if(argdata.flagNoFork)
  //  sleep(60);


#if QT_VERSION > 0x050000

#  if VTK_MAJOR_VERSION >= 8

  // We want to use multisamples everywhere by default
  vtkOpenGLRenderWindow::SetGlobalMaximumNumberOfMultiSamples(4);

  // When using VTK8 we have to set the surface format to match what it wants
  QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());

#  else

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
  // QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());

#  endif

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
  if (argdata.nDevicePixelRatio > 0)
    devicePixelRatio = argdata.nDevicePixelRatio;
  else if (getenv("ITKSNAP_SCALE_FACTOR"))
    devicePixelRatio = atoi(getenv("ITKSNAP_SCALE_FACTOR"));

  // Set the environment variable
  if (devicePixelRatio > 0)
  {
    itksnap_putenv(QT_SCALE_FACTOR, devicePixelRatio);
  }
  else
  {
    itksnap_putenv(QT_SCALE_AUTO_VAR, QT_SCALE_AUTO_VALUE);
  }

  // Turn off event debugging if needed
#ifdef SNAP_DEBUG_EVENTS
  flag_snap_debug_events = argdata.flagDebugEvents;
#endif

  // Deal with threads
  if (argdata.nThreads > 0)
  {
    itk::MultiThreaderBase::SetGlobalDefaultNumberOfThreads(argdata.nThreads);
    itk::MultiThreaderBase::SetGlobalMaximumNumberOfThreads(argdata.nThreads);
  }

  // VTK verbosity
  vtkLogger::SetStderrVerbosity(vtkLogger::VERBOSITY_WARNING);

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
  if (argdata.style != "fusion")
  {
    QPalette fpal(QColor(232, 232, 232));
    fpal.setColor(QPalette::Normal, QPalette::Highlight, QColor(70, 136, 228));
    app.setPalette(fpal);
  }

  // Test OpenGL?
  if (argdata.flagTestOpenGL)
  {
    // Set the default surface format
    auto dft = QSurfaceFormat::defaultFormat();
    QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());

    // Create two OpenGL windows
    vtkNew<vtkGenericOpenGLRenderWindow> window_1, window_2;

    // Create two renderers
    vtkNew<vtkRenderer> renderer_1, renderer_2;
    renderer_1->SetBackground(0.2, 0.2, 0.0);
    renderer_2->SetBackground(0.0, 0.2, 0.2);
    window_1->AddRenderer(renderer_1);
    window_2->AddRenderer(renderer_2);

    // Create two OpenGL widgets
    QVTKOpenGLNativeWidget *widget_1 = new QVTKOpenGLNativeWidget();
    QVTKOpenGLNativeWidget *widget_2 = new QVTKOpenGLNativeWidget();

    // Hook up widgets to windows
    widget_1->setRenderWindow(window_1.Get());
    widget_2->setRenderWindow(window_2.Get());

    // Place a sphere in window 1
    vtkNew<vtkSphereSource> sphereSource;
    sphereSource->SetCenter(0.0, 0.0, 0.0);
    sphereSource->SetRadius(5.0);
    sphereSource->SetPhiResolution(20);
    sphereSource->SetThetaResolution(20);

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(sphereSource->GetOutputPort());

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(1.0, 0.0, 0.0);
    renderer_1->AddActor(actor);

    // Place some text in window 2
    vtkNew<vtkTextActor> txt;
    txt->SetInput("Hello World");
    txt->SetPosition(10, 10);
    renderer_2->AddActor2D(txt);

    // Place some overlay text in window 1
    vtkNew<vtkRenderer> renderer_3;
    renderer_3->SetLayer(1);
    vtkNew<vtkTextActor> txt2;
    txt2->SetInput("Overlay");
    txt2->SetPosition(10, 10);
    renderer_3->AddActor2D(txt2);
    window_1->AddRenderer(renderer_3);
    window_1->SetNumberOfLayers(2);

    auto *dlg = new QDialog();
    auto *lo = new QHBoxLayout(dlg);
    lo->addWidget(widget_1);
    lo->addWidget(widget_2);
    lo->addWidget(new QPushButton("Ok"));
    widget_1->setMinimumSize(QSize(256, 256));
    widget_2->setMinimumSize(QSize(256, 256));
    dlg->setModal(false);
    dlg->show();
  }

  // Before we can create any of the framework classes, we need to get some
  // platform-specific functionality to the SystemInterface
  QtSystemInfoDelegate siDelegate;
  SystemInterface::SetSystemInfoDelegate(&siDelegate);

  // We also need to create the Qt-based object that handles shared memory communication
  // and pass it to the appropriate model
  QtSharedMemorySystemInterface siSharedMem;

  // Create the global UI
  try
  {
    SmartPtr<GlobalUIModel> gui = GlobalUIModel::New();
    IRISApplication        *driver = gui->GetDriver();

    // Pass the shared memory interface to its model
    gui->GetSynchronizationModel()->SetSystemInterface(&siSharedMem);
    gui->GetSynchronizationModel()->SetDebugSync(argdata.flagDebugSync);

    // Set the initial directory. The fallthough is to set to the user's home
    // directory
    QString init_dir = QDir::homePath();
    QString app_dir = QApplication::applicationDirPath();

    // Also get the directory one up from the application dir (this is because
    // on windows "run in" defaults to one up dir)
    QDir app_up_qdir(app_dir);
    app_up_qdir.cdUp();
    QString app_up_dir = app_up_qdir.path();

    // If the user provides a flag for the current directory, try using it but
    // only if this is a valid directory
    if (argdata.cwd.size())
    {
      QDir dir(from_utf8(argdata.cwd));
      if (dir.exists() && dir.isReadable())
      {
        init_dir = dir.absolutePath();
      }
    }
    else if (QDir::currentPath().length() > 1 && QDir::currentPath() != app_dir &&
             QDir::currentPath() != app_up_dir)
    {
      init_dir = QDir::currentPath();
    }

    gui->GetGlobalState()->SetInitialDirectory(to_utf8(init_dir));

    // Load the user preferences
    gui->LoadUserPreferences();

    // Print locale information
    if (argdata.gui_language.size())
    {
      QLocale::setDefault(QLocale(argdata.gui_language.c_str()));
      qDebug() << "ITK-SNAP using locale:" << QLocale().name();
    }
    QTranslator translator;
    if(translator.load(QLocale(), "itksnap", "_", ":/i18n"))
    {
      qDebug() << "Loaded translator for locale" << QLocale().name();
      QCoreApplication::installTranslator(&translator);
    }
    else
    {
      qDebug() << "Failed to load translator for locale" << QLocale().name();
    }

    // Create the main window
    MainImageWindow *mainwin = new MainImageWindow();
    mainwin->Initialize(gui);

    // Load stylesheet
    if (argdata.cssfile.size())
    {
      QFileSystemWatcher *watcher = new QFileSystemWatcher(mainwin);
      watcher->addPath(from_utf8(argdata.cssfile));
      QObject::connect(
        watcher, SIGNAL(fileChanged(QString)), mainwin, SLOT(externalStyleSheetFileChanged(QString)));
    }

    // Disable double buffering in X11 to avoid flickering issues. The documentation
    // says this only happens on X11. For the time being, we are only implementing this
    // for Qt4 and X11
#if QT_VERSION < 0x050000
#  ifdef Q_WS_X11
    if (!argdata.flagX11DoubleBuffer)
      mainwin->setAttribute(Qt::WA_PaintOnScreen);
#  endif
#endif

    // Start parsing options
    IRISWarningList warnings;


    // Check if a workspace is being loaded
    if (argdata.fnWorkspace.size())
    {
      // Put a waiting cursor
      QtCursorOverride curse(Qt::WaitCursor);

      // Load the workspace
      try
      {
        driver->OpenProject(argdata.fnWorkspace, warnings);
      }
      catch (std::exception &exc)
      {
        ReportNonLethalException(
          mainwin,
          exc,
          "Workspace Error",
          QString("Failed to load workspace %1").arg(from_utf8(argdata.fnWorkspace)));
      }
    }
    else
    {
      // Load main image file
      if (argdata.fnMain.size())
      {
        // Put a waiting cursor
        QtCursorOverride curse(Qt::WaitCursor);

        // Try loading the image
        try
        {
          // Load the main image. If that fails, all else should fail too
          driver->OpenImage(argdata.fnMain.c_str(), MAIN_ROLE, warnings);

          // Load the segmentation
          if (argdata.fnSegmentation.size())
          {
            std::string current_seg;
            try
            {
              for (int i = 0; i < argdata.fnSegmentation.size(); ++i)
              {
                current_seg = argdata.fnSegmentation[i];
                driver->OpenImage(current_seg.c_str(), LABEL_ROLE, warnings, nullptr, nullptr, i > 0);
              }
            }
            catch (std::exception &exc)
            {
              ReportNonLethalException(
                mainwin,
                exc,
                "Image IO Error",
                QString("Failed to load segmentation %1").arg(from_utf8(current_seg)));
            }
          }

          // Load the overlays
          if (argdata.fnOverlay.size())
          {
            std::string current_overlay;
            try
            {
              for (int i = 0; i < argdata.fnOverlay.size(); i++)
              {
                current_overlay = argdata.fnOverlay[i];
                driver->OpenImage(current_overlay.c_str(), OVERLAY_ROLE, warnings);
              }
            }
            catch (std::exception &exc)
            {
              ReportNonLethalException(
                mainwin,
                exc,
                "Overlay IO Error",
                QString("Failed to load overlay %1").arg(from_utf8(current_overlay)));
            }
          }

          // Load the meshes
          if (argdata.fnMesh.size())
          {
            std::string current_mesh;
            try
            {
              auto *model = gui->GetMeshImportModel();
              for (int i = 0; i < argdata.fnMesh.size(); i++)
              {
                current_mesh = argdata.fnMesh[i];
                std::string ext = current_mesh.substr(current_mesh.find_last_of("."));
                auto fmt = GuidedMeshIO::GetFormatByExtension(ext);
                std::vector<std::string> fn_list { current_mesh };
                if (fmt != GuidedMeshIO::FORMAT_COUNT)
                {
                  std::cout << "Loading mesh " << current_mesh << std::endl;
                  model->Load(fn_list, fmt, 1);
                }
              }
            }
            catch (std::exception &exc)
            {
              ReportNonLethalException(
                mainwin,
                exc,
                "Mesh IO Error",
                QString("Failed to load mesh %1").arg(from_utf8(current_mesh)));
            }
          }
        }
        catch (std::exception &exc)
        {
          ReportNonLethalException(mainwin,
                                   exc,
                                   "Image IO Error",
                                   QString("Failed to load image %1").arg(from_utf8(argdata.fnMain)));
        }
      } // if main image filename supplied

      if (argdata.fnLabelDesc.size())
      {
        try
        {
          // Load the label file
          driver->LoadLabelDescriptions(argdata.fnLabelDesc.c_str());
        }
        catch (std::exception &exc)
        {
          ReportNonLethalException(
            mainwin,
            exc,
            "Label Description IO Error",
            QString("Failed to load labels from %1").arg(from_utf8(argdata.fnLabelDesc)));
        }
      }
    } // Not loading workspace

    // Zoom level
    if (argdata.xZoomFactor > 0)
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
    if (argdata.geometry[0] > 0 && argdata.geometry[1] > 0)
    {
      mainwin->resize(QSize(argdata.geometry[0], argdata.geometry[1]));
      if (argdata.geometry[2] >= 0 && argdata.geometry[3] >= 0)
      {
        mainwin->move(argdata.geometry[2], argdata.geometry[3]);
      }
    }

    // Show the panel
    mainwin->ShowFirstTime();

    // Skip these checks when testing, or it will interrupt the automation if not handled correctly
    bool ui_testing = argdata.xTestId.size() > 0;
    if (!ui_testing)
    {
      // Check for updates?
      mainwin->UpdateAutoCheck();

      // Remind layout preference
      mainwin->RemindLayoutPreference();

      // Set up crash recovery
      std::set_terminate(test_terminate_handler);
    }

    // Assign the main window to the application. We do this right before
    // starting the event loop.
    app.setMainWindow(mainwin);

    // Do the test
    if (ui_testing)
    {
      testingEngine = new SNAPTestQt(mainwin, argdata.fnTestDir, argdata.xTestAccel);
      testingEngine->LaunchTest(argdata.xTestId);
    }

    // TODO: remove this
    /*
    QPalette p = QGuiApplication::palette();
    int roles[] = { QPalette::WindowText, QPalette::Button, QPalette::Light, QPalette::Midlight,
    QPalette::Dark, QPalette::Mid, QPalette::Text, QPalette::BrightText, QPalette::ButtonText,
    QPalette::Base, QPalette::Window, QPalette::Shadow, QPalette::Highlight,
    QPalette::HighlightedText, QPalette::Link, QPalette::LinkVisited, QPalette::AlternateBase,
              QPalette::NoRole,
              QPalette::ToolTipBase, QPalette::ToolTipText,
              QPalette::PlaceholderText };
    const char *role_names[] = { "WindowText", "Button", "Light", "Midlight", "Dark", "Mid",
                   "Text", "BrightText", "ButtonText", "Base", "Window", "Shadow",
                   "Highlight", "HighlightedText",
                   "Link", "LinkVisited",
                   "AlternateBase",
                   "NoRole",
                   "ToolTipBase", "ToolTipText",
                   "PlaceholderText" };

    for(unsigned int i = 0; i < 21; i++)
      qDebug() << role_names[i] << ":" << p.color((QPalette::ColorRole)roles[i]);
    */

    // Run application
    int rc;
    try
    {
      rc = app.exec();
    }
    catch (std::exception &exc)
    {
      test_terminate_handler();
      rc = -1;
    }

    // If everything cool, save the preferences, but not when testing because preferences
    // set in test mode should not be kept
    if (!rc && !ui_testing)
      gui->SaveUserPreferences();

    // Unload the main image before all the destructors start firing
    driver->UnloadMainImage();

    // Get rid of the main window while the model is still alive
    delete mainwin;

    // Destroy the model after the GUI is destroyed
    gui = NULL;

    // Destory the test engine
    if (testingEngine)
      delete testingEngine;

    // Exit with the return code
    std::cerr << "Return code : " << rc << std::endl;
    return rc;
  }
  catch (std::exception &exc)
  {
    ReportNonLethalException(
      NULL, exc, "ITK-SNAP failed to start", "Exception occurred during ITK-SNAP startup");
    exit(-1);
  }
}
