//
// test-browser.cxx -- test the Fl_Native_File_Chooser widget
//
// Copyright 2004 by Greg Ercolano.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please keep code 80 column compliant.
//
//      10        20        30        40        50        60        70
//       |         |         |         |         |         |         |
// 4567890123456789012345678901234567890123456789012345678901234567890123456789
//
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Multiline_Input.H>
#include <FL/Fl_Multiline_Output.H>
#include <FL/Fl_Value_Input.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Multi_Browser.H>
#include <FL/Fl_Native_File_Chooser.H>

#ifdef _WIN32
// WINDOWS //
#include <direct.h>
#define getcwd _getcwd
#define MAXPATHLEN MAX_PATH
#else
// UNIX //
#include <sys/param.h>
#include <unistd.h>
#include <stdlib.h>
#endif

// GLOBALS
Fl_Native_File_Chooser *G_choo = NULL;
Fl_Hold_Browser        *G_type = NULL;
Fl_Multi_Browser       *G_options = NULL;
Fl_Input               *G_directory = NULL;
Fl_Input               *G_title = NULL;
Fl_Input               *G_preset_file = NULL;
Fl_Multiline_Output    *G_result = NULL;
Fl_Multiline_Input     *G_filter = NULL;
Fl_Value_Input         *G_filter_value = NULL;

void Butt_CB(Fl_Widget*w, void*) {

    // TYPE OF CHOOSER
    switch ( G_type->value() ) {
	case 1: G_choo->type(Fl_Native_File_Chooser::BROWSE_FILE);            break;
	case 2: G_choo->type(Fl_Native_File_Chooser::BROWSE_DIRECTORY);       break;
	case 3: G_choo->type(Fl_Native_File_Chooser::BROWSE_MULTI_FILE);      break;
	case 4: G_choo->type(Fl_Native_File_Chooser::BROWSE_MULTI_DIRECTORY); break;
	case 5: G_choo->type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);       break;
    }

    // OPTIONS
    {
	int flags = 0;
	if ( G_options->selected(1) ) 
	    flags |= Fl_Native_File_Chooser::SAVEAS_CONFIRM;
	if ( G_options->selected(2) )
	    flags |= Fl_Native_File_Chooser::NEW_FOLDER;
	if ( G_options->selected(3) )
	    flags |= Fl_Native_File_Chooser::PREVIEW;
	G_choo->options(flags);
    }

    // DIRECTORY
    if ( G_directory->value()[0] ) {
	if ( strcmp(G_directory->value(), "NULL") == 0 ) {
	    G_choo->directory(NULL);
	} else {
	    G_choo->directory(G_directory->value());
	}
    } else {
	G_choo->directory(NULL);
    }

    // PRESET FILE
    if ( G_preset_file->value()[0] ) {
	if ( strcmp(G_preset_file->value(), "NULL") == 0 ) {
	    G_choo->preset_file(NULL);
	} else {
	    G_choo->preset_file(G_preset_file->value());
	}
    } else {
	G_choo->preset_file(NULL);
    }

    // TITLE
    if ( G_title->value()[0] ) {
	if ( strcmp(G_title->value(), "NULL") == 0 ) {
	    G_choo->title(NULL);
	} else {
	    G_choo->title(G_title->value());
	}
    } else {
	G_choo->title("");
    }

    // FILTERS
    if ( G_filter->value()[0] ) {
	if ( strcmp(G_filter->value(), "NULL") == 0 ) {
	    G_choo->filter(NULL);
	} else {
	    G_choo->filter(G_filter->value());
	}
    } else {
	G_choo->filter("");		// all files
    }

    G_choo->filter_value((int)G_filter_value->value());

    switch ( G_choo->show() ) {
	case -1:	// ERROR
	    fprintf(stderr, "*** ERROR show():%s\n", G_choo->errmsg());
	    break;

	case 1:		// CANCEL
	    fprintf(stderr, "*** CANCEL\n");
	    break;

	default:
	    break;
    }

    // UPDATE SETTINGS RETURNED BY CHOOSER INTO BROWSER

    // RESULT
    {
	int count = G_choo->count();
	int bytes = 0;
	if ( count > 0 ) {
	    int t;
	    for ( t=0; t<count; t++ ) {
	        bytes += strlen(G_choo->filename(t)) + 2;
		if ( count > 1 )
		    fprintf(stderr, "MULTI FILENAME RESULT %d) '%s'\n",
			t, G_choo->filename(t));
		else
		    fprintf(stderr, "FILENAME RESULT '%s'\n",
			G_choo->filename(t));
	    }
	    char *s = new char[bytes];
	    s[0] = '\0';
	    for ( t=0; t<count; t++ ) {
	        strcat(s, G_choo->filename(t));
		strcat(s, "\n");
	    }
	    G_result->value(s);
	    delete [] s;
	}
    }

    // FILTER VALUE
    G_filter_value->value(G_choo->filter_value());
    fprintf(stderr, "FILTER VALUE USED: %d\n", G_choo->filter_value());

    // PRESET FILE
    {
	const char *s = G_choo->preset_file();
	G_preset_file->value(s ? s : "NULL");
	fprintf(stderr, "    PRESET_FILE(): %s\n", G_choo->preset_file());
    }

    // DIRECTORY
    {
	const char *s = G_choo->directory();
	G_directory->value(s ? s : "NULL");
	fprintf(stderr, "      DIRECTORY(): %s\n", G_choo->directory());
    }

    fprintf(stderr, "\n");
}

int main() {
    
//  char *start_filter = "Text\t*.txt\nC Files\t*.{cxx,h,c}";
    char *start_filter =
      "Source Code\t*.{cxx,h,H,cpp}\n"\
      "Cxx Only\t*.cxx\n"\
      "Text\t*.{txt}\n"\
      "Makefiles\tMakefile*\n"\
      "All image files\t*.{BMP,CUT,DDS,GIF,ICO,IFF,LBM,JNG,JPG,JPEG,JPE,JIF,KOA,"
                          "MNG,PBM,PCD,PCX,PGM,PNG,PPM,PSD,RAS,TGA,TIF,TIFF,WBMP,"
                          "XBM,XPM}\n"\
      "Windows, OS/2 Bitmap (*.BMP)\t*.bmp\n"\
      "Dr. Halo (*.CUT)\t*.CUT\n"\
      "DirectDraw Surface (*.DDS)\t*.DDS\n"\
      "Graphic Interchange Format (*.GIF)\t*.GIF\n"\
      "Windows Icon (*.ICO)\t*.ICO\n"\
      "Amiga IFF (*.IFF;*.LBM)\t*.{IFF,LBM}\n"\
      "JBIG (*.JBIG)\t*.JBIG\n"\
      "JPEG Network Graphics (*.JNG)\t*.JNG\n"\
      "Independent JPEG Group (*.JPG;*.JPEG;*.JPE;*.JIF)\t*.{JPG,JIF,JPEG,JPE}\n"\
      "Commodore 64 Koala (*.KOA)\t*.KOA\n"\
      "Multiple Network Graphics (*.MNG)\t*.MNG\n"\
      "Portable Bitmap (*.PBM)\t*.PBM\n"\
      "Kodak PhotoCD (*.PCD)\t*.PCD\n"\
      "PC Paintbrush Bitmap (*.PCX)\t*.PCX\n"\
      "Portable Graymap (*.PGM)\t*.PGM\n"\
      "Portable Network Graphics (*.PNG)\t*.PNG\n"\
      "Portable Pixelmap (*.PPM)\t*.PPM\n"\
      "Photoshop Document (*.PSD)\t*.PSD\n"\
      "Sun Raster Graphic (*.RAS)\t*.RAS\n"\
      "Targa (*.TGA)\t*.TGA\n"\
      "Tagged Image File Format (*.TIF;*.TIFF)\t*.{TIF,TIFF}\n"\
      "Wireless Bitmap (*.WBMP)\t*.WBMP\n"\
      "X11 Bitmap (*.XBM)\t*.XBM\n"\
      "X11 Pixmap (*.XPM)\t*.XPM";

    char start_dir[MAXPATHLEN + 1];
    getcwd(start_dir, MAXPATHLEN);

    char start_file[MAXPATHLEN + 1];
    strcpy(start_file, "testfile");

    Fl_Window *win = new Fl_Window(600, 520, "Test Browser (FLTK1)");

    int y = 20;

    G_type = new Fl_Hold_Browser(10,y,180,120,"Type");
    G_type->add("Single File");
    G_type->add("Single Directory");
    G_type->add("Multi File");
    G_type->add("Multi Directory");
    G_type->add("Save File");
    G_type->textsize(12);
    G_type->value(1);
    G_type->tooltip("Type of browser to use");

    G_options = new Fl_Multi_Browser(win->w()-180-10,y,180,120,"Options");
    G_options->add("Show SaveAs Confirm");
    G_options->add("Show New Folder");
    G_options->add("Show Previews");
    G_options->tooltip("Platform specific options.\nSeveral can be selected.");
    G_options->textsize(12);

    y += G_type->h() + 10 + 20;

    G_title = new Fl_Input(80, y, win->w()-80-10, 25, "Title");
    G_title->value("Title Of Window");
    G_title->tooltip("Sets title of browser window\n"
                     "Set this to 'NULL' for a NULL setting");
    y += G_title->h() + 5;

    G_directory = new Fl_Input(80, y, win->w()-80-10, 25, "Directory");
    G_directory->value(start_dir);
    G_directory->tooltip("Starting directory shown in browser\n"
                         "Set this to 'NULL' for a NULL setting");
    y += G_directory->h() + 5;

    G_preset_file = new Fl_Input(80, y, win->w()-80-10, 25, "Preset File");
    G_preset_file->value(start_file);
    G_preset_file->tooltip("Default filename used for 'Save File' chooser\n"
                           "Set this to 'NULL' for a NULL setting");
    y += G_preset_file->h() + 5;

    G_filter = new Fl_Multiline_Input(80, y, win->w()-80-10, 75, "Filter");
    G_filter->value(start_filter);
    G_filter->tooltip("Filter(s) to be available to user\n"
		      "Multiple filters should be specified on separate lines\n"
		      "Set this to 'NULL' for a NULL setting");
    y += G_filter->h() + 5;

    G_filter_value = 
        new Fl_Value_Input(80, y, win->w()-80-10, 25, "Filter Value");
    G_filter_value->value(0);
    G_filter_value->tooltip("Index number of the filter to be applied "
                            "when browser opens.");
    y += G_filter_value->h() + 5;

    G_result = new Fl_Multiline_Output(80, y, win->w()-80-10, 100, "Result");
    G_result->color(51);

    Fl_Button *but = 
        new Fl_Button(win->w()-80-10, win->h()-25-10, 80, 25, "Browser");
    but->callback(Butt_CB);
    y += but->h() + 10;

    G_choo = new Fl_Native_File_Chooser();
    G_choo->type(Fl_Native_File_Chooser::BROWSE_FILE);

    win->resizable(win);
    win->show();
    return(Fl::run());
}
