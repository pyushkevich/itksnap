/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SNAPMain.cxx,v $
  Language:  C++
  Date:      $Date: 2011/05/04 15:25:42 $
  Version:   $Revision: 1.23 $
  Copyright (c) 2007 Paul A. Yushkevich
  
  This file is part of ITK-SNAP 

  ITK-SNAP is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  -----

  Copyright (c) 2003 Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information. 

=========================================================================*/
// Borland compiler is very lazy so we need to instantiate the template
//  by hand 
#if defined(__BORLANDC__)
#include "SNAPBorlandDummyTypes.h"
#endif

#include "FL/Fl.H"
#include "FL/Fl_Native_File_Chooser.H"
#include <FL/Fl_File_Chooser.H>

#include "CommandLineArgumentParser.h"
#include "ImageCoordinateGeometry.h"
#include "ImageIORoutines.h"
#include "IRISApplication.h"
#include "IRISException.h"
#include "IRISImageData.h"
#include "SNAPRegistryIO.h"
#include "SystemInterface.h"
#include "UserInterfaceLogic.h"

#include "itkOrientedImage.h"
#include "itkImageFileReader.h"
#include "itkNumericTraits.h"


#include <itksys/SystemTools.hxx>

//#include <limits>

using namespace std;

// Define the verbose output stream
ostream &verbose = cout;

// MAX grey value - TODO find somewhere to stick this
const GreyType MAXGREYVAL = itk::NumericTraits<GreyType>::max();
const GreyType MINGREYVAL = itk::NumericTraits<GreyType>::min();

#include "Registry.h"

// A templated load image method
template<class TPixel>
bool LoadImageFromFileInteractive(
  const char *file, typename itk::SmartPointer< itk::OrientedImage<TPixel,3> > &target)
{
  try
    {
    LoadImageFromFile(file,target);
    return true;
    }
  catch(itk::ExceptionObject &exc)
    {
    cerr << "Error loading file '" << file << "'" << endl;
    cerr << "Reason: " << exc << endl;
    return false;
    }
}

/** Make sure we have a path to the SNAP root directory 
 * so that we can read external files */
bool FindDataDirectoryInteractive(const char *sExePath, SystemInterface &system)
{
  // Try to find the root directory
  while(!system.FindDataDirectory(sExePath))
    {
    // Tell the user the directory is missing
    int choice = 
      fl_choice("The SNAP root directory could not be found!\n"
                "Please click 'Search' to find this directory\n"
                "or click 'Exit' to terminate the program.",
                "Exit Program","Browse...","More Info...");

    if(choice == 0)
      {
      // Exit the application
      return false;
      }      
    else if(choice == 2)
      {
      // More help
      fl_message("  SNAP could not find its help files and other data that it needs\n"
                 "to run properly.  These files should be located in a subdirectory \n"
                 "called ProgramData in the same directory where the SNAP executable \n"
                 "is located.  \n"
                 "  If you are not sure where to look, search your computer's hard drive \n"
                 "for a file called '%s', and then use the Browse... button to tell SNAP \n"
                 "where this file is located.\n"
                 "  For more help, consult your system administrator.",
                 system.GetProgramDataDirectoryTokenFileName());
      }
    else
      {
      // The file we're looking for
      string sMissingFile = system.GetProgramDataDirectoryTokenFileName();
      string sTitle = "Find a directory that contains file " + sMissingFile;

      // Look for the file using a file chooser
      Fl_Native_File_Chooser fc;
      fc.type(Fl_Native_File_Chooser::BROWSE_FILE);
      fc.title(sTitle.c_str());
      fc.filter("Directory Token File\t*.txt");
      fc.preset_file(sMissingFile.c_str());
          
      // Show the file chooser
      const char *fName = NULL;
      if (fc.show())
        {
        fName = fc.filename();
        }
      
      // If user hit cancel, continue
      if(!fName || !strlen(fName)) continue;

      // Make sure that the filename matches and get the path
      string sBrowseName = itksys::SystemTools::GetFilenameName(fName);
             
      // If not, check if they've selected a valid directory
      if(sBrowseName != sMissingFile)
        {
        // Wrong directory specified!
        fl_alert("The directory must contain file '%s'!",sMissingFile.c_str());
        continue;
        }

      // Make sure that the filename matches and get the path
      string sBrowsePath = itksys::SystemTools::GetFilenamePath(fName);

      // Set the path
      system["System.ProgramDataDirectory"] << sBrowsePath;
      }
    }
  return true;
}

bool LoadUserPreferencesInteractive(SystemInterface &system) 
{
  try
    {
    system.LoadUserPreferences();
    }
  catch(std::string &exc)
    {
    if(0 == fl_choice("Error reading preferences file %s\n%s",
                      "Exit Program","Continue",NULL,
                      system.GetUserPreferencesFileName(),exc.c_str()))
      {
      return false;
      }   
    }

  return true;
}

// Setup printing of stack trace on segmentation faults. This only
// works on select GNU systems
#if defined(__GNUC__) && !defined(__CYGWIN__) && !defined(__APPLE__) && !defined(sun) && !defined(WIN32)

#include <signal.h>
#include <execinfo.h>

void SegmentationFaultHandler(int sig)
{
  cerr << "*************************************" << endl;
  cerr << "ITK-SNAP: Segmentation Fault!   " << endl;
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


void usage()
{
  // Print usage info and exit
  cout << "ITK-SnAP Command Line Usage:" << endl;
  cout << "   snap [options] [main_image]" << endl;
  
  cout << "Options:" << endl;

  cout << "   --main, -m FILE              : " <<
    "Load main image FILE; automatically determine grey or RGB" << endl;
  
  cout << "   --grey, -g FILE              : " <<
    "Load main image FILE as greyscale" << endl;
  
  cout << "   --rgb FILE                   : " <<
    "Load main image FILE as RGB image" << endl;

  cout << "   --segmentation, -s FILE      : " <<
    "Load segmentation image FILE" << endl;
  
  cout << "   --labels, -l FILE            : " <<
    "Load label description file FILE" << endl;

  cout << "   --overlay, -o FILE           : " <<
    "Load overlay image FILE; automatically determine grey or RGB" << endl;
  
  cout << "   --compact <a|c|s>            : " <<
    "Launch in compact single-slice mode (axial, coronal, sagittal)" << endl;

  cout << "   --zoom, -z FACTOR            : " <<
    "Specify initial zoom in screen pixels / physical mm" << endl;
}
    

// creates global pointers
// sets up the GUI and lets things run
int main(int argc, char **argv) 
{
  // Handle signals gracefully, with trace-back
  SetupSignalHandlers();

  // Turn off ITK warning windows
  itk::Object::GlobalWarningDisplayOff();

  // Parse command line parameters
  CommandLineArgumentParser parser;
  parser.AddOption("--grey",1);
  parser.AddSynonim("--grey","-g");

  parser.AddOption("--main",1);
  parser.AddSynonim("--main","-m");

  parser.AddOption("--rgb", 1);

  parser.AddOption("--segmentation",1);
  parser.AddSynonim("--segmentation","-s");
  parser.AddSynonim("--segmentation","-seg");
  
  parser.AddOption("--overlay", 1);
  parser.AddSynonim("--overlay", "-o");

  parser.AddOption("--labels",1);
  parser.AddSynonim("--labels","--label");
  parser.AddSynonim("--labels","-l");

  parser.AddOption("--zoom", 1);
  parser.AddSynonim("--zoom", "-z");

  parser.AddOption("--compact", 1);
  parser.AddSynonim("--compact", "-c");

  parser.AddOption("--help", 0);
  parser.AddSynonim("--help", "-h");


  CommandLineArgumentParseResult parseResult;
  int iTrailing = 0;
  if(!parser.TryParseCommandLine(argc,argv,parseResult,false,iTrailing))
    {
    cerr << "Unable to parse command line. Run " << argv[0] << " -h for help" << endl;
    return -1;
    }

  if(parseResult.IsOptionPresent("--help"))
    {
    usage();
    return 0;
    }

  // Create a new IRIS application
  IRISApplication *iris = new IRISApplication;

  // Initialize the operating system interface
  SystemInterface &system = *iris->GetSystemInterface();  

  // Load the user preferences into the system
  if(!LoadUserPreferencesInteractive(system))
    return -1;

  // Work with the user to try to find the root directory
  if(!FindDataDirectoryInteractive(argv[0],system))
    return -1;

  // Create a UI object
  UserInterfaceLogic *ui = new UserInterfaceLogic(iris);

  // Initialize FLTK
  Fl::visual(FL_DOUBLE|FL_INDEX);
  Fl::gl_visual(FL_RGB);  
  Fl::background(236,233,216);

  // Show the IRIS Interface
  ui->Launch();

  // Show the splash screen
  ui->ShowSplashScreen();

  // The following situations are possible for main image
  // itksnap file                       <- load as main image, detect file type
  // itksnap --main file                <- load as main image, detect file type
  // itksnap --gray file                <- load as main image, force gray
  // itksnap --rgb file                 <- load as main image, force RGB
  // itksnap --gray file1 --rgb file2   <- error
  // itksnap --gray file1 file2         <- ignore file2
  // itksnap --rgb file1 file2          <- ignore file2

  // Check validity of options for main image
  if(parseResult.IsOptionPresent("--grey") && parseResult.IsOptionPresent("--rgb"))
    {
    cerr << "Error: options --rgb and --grey are mutually exclusive." << endl;
    return -1;
    }

  if(parseResult.IsOptionPresent("--main") && parseResult.IsOptionPresent("--rgb"))
    {
    cerr << "Error: options --main and --rgb are mutually exclusive." << endl;
    return -1;
    }

  if(parseResult.IsOptionPresent("--main") && parseResult.IsOptionPresent("--grey"))
    {
    cerr << "Error: options --main and --grey are mutually exclusive." << endl;
    return -1;
    }

  // Check if a main image file is specified 
  bool force_grey = false, force_rgb = false;
  const char *fnMain = NULL;
  if(parseResult.IsOptionPresent("--main"))
    {
    fnMain = parseResult.GetOptionParameter("--main");
    }
  else if(parseResult.IsOptionPresent("--grey"))
    {
    fnMain = parseResult.GetOptionParameter("--grey");
    force_grey = true;
    }
  else if(parseResult.IsOptionPresent("--rgb")) 
    {
    fnMain = parseResult.GetOptionParameter("--rgb");
    force_rgb = true;
    }
  else if(iTrailing < argc)
    {
    fnMain = argv[iTrailing];
    }


  // If no main, there should be no overlays, segmentation
  if(!fnMain && parseResult.IsOptionPresent("--segmentation"))
    {
    cerr << "Error: --segmentation can not be used without --main, --grey, or --rgb" << endl;
    return -1;
    }
  
  if(!fnMain && parseResult.IsOptionPresent("--overlay"))
    {
    cerr << "Error: --overlay can not be used without --main, --grey, or --rgb" << endl;
    return -1;
    }

  // Load main image file
  if(fnMain)
    {
    // Update the splash screen
    ui->UpdateSplashScreen("Loading image...");

    // Try loading the image
    try 
      {
      ui->NonInteractiveLoadMainImage(fnMain, force_grey, force_rgb);
      }  
    catch(itk::ExceptionObject &exc)
      {
      cerr << "Error loading file '" << fnMain << "'" << endl;
      cerr << "Reason: " << exc << endl;
      return -1;
      }
    }

  // If one main image loaded, load segmentation
  if(fnMain)
    {
    // Load the segmentation if supplied
    if(parseResult.IsOptionPresent("--segmentation"))
      {
      // Get the filename 
      const char *fname = parseResult.GetOptionParameter("--segmentation");

      // Update the splash screen
      ui->UpdateSplashScreen("Loading segmentation image...");

      // Try to load the image
      try
        {
        ui->NonInteractiveLoadSegmentation(fname);
        }
      catch(itk::ExceptionObject &exc)
        {
        cerr << "Error loading file '" << fname << "'" << endl;
        cerr << "Reason: " << exc << endl;
        return -1;
        }
      }    

    // Load overlay is supplied
    if(parseResult.IsOptionPresent("--overlay"))
      {
      // Get the filename
      const char *fname = parseResult.GetOptionParameter("--overlay");

      // Update the splash screen
      ui->UpdateSplashScreen("Loading overlay image...");

      // Try to load the image
      try
        {
        ui->NonInteractiveLoadOverlayImage(fname, false, false);
        }
      catch(itk::ExceptionObject &exc)
        {
        cerr << "Error loading file '" << fname << "'" << endl;
        cerr << "Reason: " << exc << endl;
        return -1;
        }
      }
    }

  // Load labels if supplied
  if(parseResult.IsOptionPresent("--labels"))
    {
    // Get the filename 
    const char *fname = parseResult.GetOptionParameter("--labels");
    
    // Update the splash screen
    ui->UpdateSplashScreen("Loading label descriptions...");

    try 
      {
      // Load the label file
      ui->NonInteractiveLoadLabels(fname);
      }
    catch(itk::ExceptionObject &exc)
      {
      cerr << "Error reading label descriptions: " << 
        exc.GetDescription() << endl;
      }
    }

  // Set initial zoom if specified
  if(parseResult.IsOptionPresent("--zoom"))
    {
    double zoom = atof(parseResult.GetOptionParameter("--zoom"));
    cout << "Using ZOOM " << zoom << endl;
    if(zoom >= 0.0)
      {
      ui->SetZoomLevelAllWindows(zoom);
      }
    else
      {
      cerr << "Invalid zoom level (" << zoom << ") specified" << endl;
      }
    }

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

  // Show the welcome message
  ui->UpdateSplashScreen("Welcome to SnAP!");
    
  // Show the splash screen
  ui->HideSplashScreen();

  // Run the UI
  try
    {
    Fl::run();
    }
  catch(std::exception &exc)
    {
    int salvage = fl_choice(
      "A run-time exception has occurred. ITK-SNAP must terminate.\n\n"
      "The text of the exception follows:\n%s\n\n"
      "ITK-SNAP can try to save your segmentation image before exiting.\n"
      "Do you wish to do so?", 
      "No, quit without saving!", "Yes, try saving segmentation!", NULL,
      exc.what());
    if(salvage)
      {
      char *fnsave = fl_file_chooser(
        "Select a filename to save segmentation", 
        "*.nii", "recovered_segmentation.nii");
      if(fnsave)
        {
        try 
          {
          typedef itk::ImageFileWriter<LabelImageWrapper::ImageType> WriterType;
          WriterType::Pointer writer = WriterType::New();
          writer->SetInput(
            ui->GetDriver()->GetIRISImageData()->GetSegmentation()->GetImage());
          writer->SetFileName(fnsave);
          writer->Update();
          }
        catch(...)
          {
          fl_alert("Failed to save segmentation image. Unfortunately, your work was lost.");
          }
        }
      }
    }

  // Write the user's preferences to disk
  try 
    {
    system.SaveUserPreferences();
    }
  catch(std::string &exc)
    {
    fl_alert("Failed to write preferences to file %s\n%s",
             system.GetUserPreferencesFileName(),exc.c_str());
    }
  
  // Delete the UI object
  delete ui;

  // Terminate the application
  delete iris;
  
  return 0;
}

/*
 *$Log: SNAPMain.cxx,v $
 *Revision 1.23  2011/05/04 15:25:42  pyushkevich
 *Fixes to build with fltk 1.3.0rc3
 *
 *Revision 1.22  2010/04/16 04:02:35  pyushkevich
 *ENH: implemented drag and drop, OSX events, new command-line interface
 *
 *Revision 1.21  2010/03/23 21:23:07  pyushkevich
 *Added display halving capability,
 *command line switches --zoom, --help, --compact
 *
 *Revision 1.20  2009/10/28 08:05:36  pyushkevich
 *FIX: Multisession pan causing continuous screen updates
 *
 *Revision 1.19  2009/10/26 20:19:12  pyushkevich
 *ENH: added top-level exception handler with option to save work
 *
 *Revision 1.18  2009/10/17 20:39:50  pyushkevich
 *ENH: added tip of the day
 *
 *Revision 1.17  2009/07/23 15:50:58  pyushkevich
 *Got the template-removal changes to compile on Linux
 *
 *Revision 1.16  2009/07/22 21:06:24  pyushkevich
 *Changed the IO system and wizards, removed templating
 *
 *Revision 1.15  2009/07/14 20:41:56  pyushkevich
 *Making Linux compilation work
 *
 *Revision 1.14  2009/06/09 05:50:04  garyhuizhang
 *ENH: main image & grey overlay support
 *
 *Revision 1.13  2009/05/25 17:09:44  garyhuizhang
 *ENH: switch from Fl_File_Chooser to Fl_Native_File_Chooser which requires the fltk to be patched with Fl_Native_File_Chooser add-on.
 *
 *Revision 1.12  2009/02/05 14:58:30  pyushkevich
 *FIX: save slice layout appearance settings to registry; ENH: added linear interpolation option for grey images
 *
 *Revision 1.11  2009/01/23 20:09:38  pyushkevich
 *FIX: 3D rendering now takes place in Nifti(RAS) world coordinates, rather than the VTK (x spacing + origin) coordinates. As part of this, itk::OrientedImage is now used for 3D images in SNAP. Still have to fix cut plane code in Window3D
 *
 *Revision 1.10  2008/11/15 12:20:38  pyushkevich
 *Several new features added for release 1.8, including (1) support for reading floating point and mapping to short range; (2) use of image direction cosines to determine image orientation; (3) new reorient image dialog and changes to the IO wizard; (4) display of NIFTI world coordinates and yoking based on them; (5) multi-session zoom; (6) fixes to the way we keep track of unsaved changes to segmentation, including a new discard dialog; (7) more streamlined code for offline loading; (8) new command-line options, allowing RGB files to be read and opening SNAP by doubleclicking images; (9) versioning for IPC communications; (10) ruler for display windows; (11) bug fixes and other stuff I can't remember
 *
 *Revision 1.9  2008/11/01 11:32:00  pyushkevich
 *Compatibility with ITK 3.8 support for reading oriented images
 *Command line loading of RGB images
 *Improved load-image commands in UserInterfaceLogic
 *
 *Revision 1.8  2007/12/30 04:43:03  pyushkevich
 *License/Packaging updates
 *
 *Revision 1.7  2007/12/26 12:26:52  pyushkevich
 *Removed the OpenGL extension thing
 *
 *Revision 1.6  2007/12/06 20:43:37  pyushkevich
 *More gentle parsing of command line arguments
 *
 *Revision 1.5  2007/09/17 20:33:09  pyushkevich
 *VTK5 compatibility
 *
 *Revision 1.4  2007/09/17 16:10:31  pyushkevich
 *Updated to VTK 5
 *
 *Revision 1.3  2007/09/17 14:22:06  pyushkevich
 *fixed slicing bug
 *
 *Revision 1.2  2007/05/11 13:06:50  pyushkevich
 *Sun compatibility fix
 *
 *Revision 1.1  2006/12/02 04:22:26  pyushkevich
 *Initial sf checkin
 *
 *Revision 1.1.1.1  2006/09/26 23:56:17  pauly2
 *Import
 *
 *Revision 1.16  2006/02/01 20:21:26  pauly
 *ENH: An improvement to the main SNAP UI structure: one set of GL windows is used to support SNAP and IRIS modes
 *
 *Revision 1.15  2005/11/07 15:50:33  pauly
 *COMP: Fixed problem with execinfo.h missing on some platforms. Also fixed
 *compilation error in GuidedImageIO.h
 *
 *Revision 1.14  2005/11/03 18:45:29  pauly
 *ENH: Enabled SNAP to read DICOM Series
 *
 *Revision 1.13  2005/10/29 14:00:14  pauly
 *ENH: SNAP enhacements like color maps and progress bar for 3D rendering
 *
 *Revision 1.12  2005/04/21 14:46:30  pauly
 *ENH: Improved management and editing of color labels in SNAP
 *
 *Revision 1.11  2005/02/04 17:01:09  lorensen
 *COMP: last of gcc 2.96 changes (I hope).
 *
 *Revision 1.10  2004/08/26 19:43:27  pauly
 *ENH: Moved the Borland code into Common folder
 *
 *Revision 1.9  2004/08/03 23:26:32  ibanez
 *ENH: Modification for building in multple platforms. By Julien Jomier.
 *
 *Revision 1.8  2004/07/09 23:07:38  pauly
 *ENH: Added a zoom-locator frame inside of the slice display window.
 *
 *Revision 1.7  2003/12/16 13:19:26  pauly
 *FIX: Removed Fl::lock()
 *
 *Revision 1.6  2003/11/25 23:32:48  pauly
 *FIX: Snake evolution did not work in multiprocessor mode
 *
 *Revision 1.5  2003/10/06 12:30:01  pauly
 *ENH: Added history lists, remembering of settings, new snake parameter preview
 *
 *Revision 1.4  2003/10/02 14:55:52  pauly
 *ENH: Development during the September code freeze
 *
 *Revision 1.3  2003/09/15 19:06:58  pauly
 *FIX: Trying to get last changes to compile
 *
 *Revision 1.2  2003/09/13 15:18:01  pauly
 *FIX: Got SNAP to work properly with different image orientations
 *
 *Revision 1.1  2003/09/11 13:51:01  pauly
 *FIX: Enabled loading of images with different orientations
 *ENH: Implemented image save and load operations
 *
 *Revision 1.3  2003/08/27 14:03:23  pauly
 *FIX: Made sure that -Wall option in gcc generates 0 warnings.
 *FIX: Removed 'comment within comment' problem in the cvs log.
 *
 *Revision 1.2  2003/08/27 04:57:47  pauly
 *FIX: A large number of bugs has been fixed for 1.4 release
 *
 *Revision 1.1  2003/07/12 04:46:50  pauly
 *Initial checkin of the SNAP application into the InsightApplications tree.
 *
 *Revision 1.1  2003/07/11 23:33:57  pauly
 **** empty log message ***
 *
 *Revision 1.8  2003/07/11 21:41:38  pauly
 *Preparation for ITK checkin
 *
 *Revision 1.7  2003/07/01 16:53:59  pauly
 **** empty log message ***
 *
 *Revision 1.6  2003/06/23 23:59:32  pauly
 *Command line argument parsing
 *
 *Revision 1.5  2003/06/14 22:42:06  pauly
 *Several changes.  Started working on implementing the level set function
 *in ITK.
 *
 *Revision 1.4  2003/05/05 12:30:18  pauly
 **** empty log message ***
 *
 *Revision 1.3  2003/04/18 17:32:18  pauly
 **** empty log message ***
 *
 *Revision 1.2  2003/04/16 05:04:17  pauly
 *Incorporated intensity modification into the snap pipeline
 *New IRISApplication
 *Random goodies
 *
 *Revision 1.1  2003/03/07 19:29:47  pauly
 *Initial checkin
 *
 *Revision 1.1.1.1  2002/12/10 01:35:36  pauly
 *Started the project repository
 *
 *
 *Revision 1.8  2002/04/01 22:27:57  moon
 *Took out global snake3D.  It's now part of SnakeVoxDataClass
 *
 *Revision 1.7  2002/03/26 19:20:13  moon
 *Changed full_data back to VoxDataClass, from SnakeVoxDataClass.  roi_data
 *is a SnakeVoxDataClass now.
 *
 *Revision 1.6  2002/03/23 02:16:37  scheuerm
 *Added subclass of VoxData called SnakeVoxData which includes
 *a preprocessed image. Doesn't do much yet but it's a start.
 *
 *Revision 1.5  2002/03/19 19:35:06  moon
 *added snakewrapper to makefile so it gets compiled. started putting in callback,
 *etc. for snake vcr buttons.  added snake object to IrisGlobals, instantiated in Main
 *
 *Revision 1.4  2002/03/08 13:54:47  moon
 *trying to add log tags
 **/
