//
// simple-app.cxx -- simple example application
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
#include <stdio.h>
#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Native_File_Chooser.H>

// GLOBALS
Fl_Input *G_filename = NULL;

void Butt_CB(Fl_Widget*, void*) {
    // Create native chooser
    Fl_Native_File_Chooser native;
    native.title("Pick a file");
    native.type(Fl_Native_File_Chooser::BROWSE_FILE);
    native.filter("Text\t*.txt\n"
                  "C Files\t*.{cxx,h,c}");
    native.preset_file(G_filename->value());
    // Show native chooser
    switch ( native.show() ) {
	case -1: fprintf(stderr, "ERROR: %s\n", native.errmsg()); break;	// ERROR
	case  1: fprintf(stderr, "*** CANCEL\n"); fl_beep(); break;		// CANCEL
	default: 								// PICKED FILE
	    if ( native.filename() )
		G_filename->value(native.filename());
	    else
		G_filename->value("NULL");
	    break;
    }
}

int main() {
    Fl_Window *win = new Fl_Window(600, 100, "FLTK Window");
    win->begin();			// (for symmetry with fltk2)
    {
	int y = 10;
	G_filename = new Fl_Input(80, y, win->w()-80-10, 25, "Filename");
	G_filename->value(".");
	G_filename->tooltip("Default filename");
	y += G_filename->h() + 5;
	Fl_Button *but = 
	    new Fl_Button(win->w()-80-10, win->h()-25-10, 80, 25, "Pick File");
	but->callback(Butt_CB);
    }
    win->end();
    win->resizable(win);
    win->show();
    return(Fl::run());
}
