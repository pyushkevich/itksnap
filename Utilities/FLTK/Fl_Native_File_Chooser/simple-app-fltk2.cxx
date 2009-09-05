//
// simple-app.cxx -- simple example application
//
// Copyright 2004 by Greg Ercolano.
// FLTK2 port by Frederic Hoerni 2007.
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
#include <fltk/run.h>
#include <fltk/ask.h>
#include <fltk/Window.h>
#include <fltk/Widget.h>
#include <fltk/Button.h>
#include <fltk/Input.h>
#include <fltk/NativeFileChooser.h>

// GLOBALS
fltk::Input *G_filename = NULL;

void Butt_CB(fltk::Widget*, void*) {
    // Create native chooser
    fltk::NativeFileChooser native;
    native.title("Pick a file");
    native.type(fltk::NativeFileChooser::BROWSE_FILE);
    native.filter("C Files\t*.{cxx,h,c}\n"
                  "C Files\t*.{cxx,h,c}");
    native.preset_file(G_filename->value());
    // Show native chooser
    switch ( native.show() ) {
	case -1: fprintf(stderr, "ERROR: %s\n", native.errmsg()); break;	// ERROR
	case  1: fprintf(stderr, "*** CANCEL\n"); fltk::beep(); break;		// CANCEL
	default: 								// PICKED FILE
	    if ( native.filename() )
		G_filename->value(native.filename());
	    else
		G_filename->value("NULL");
	    break;
    }
}

int main() {
    fltk::Window *win = new fltk::Window(600, 100, "FLTK Window");
    win->begin();
    {
	int y = 10;
	G_filename = new fltk::Input(80, y, win->w()-80-10, 25, "Filename");
	G_filename->value(".");
	G_filename->tooltip("Default filename");
	y += G_filename->h() + 5;
	fltk::Button *but = 
	    new fltk::Button(win->w()-80-10, win->h()-25-10, 80, 25, "Pick File");
	but->callback(Butt_CB);
    }
    win->end();
    win->resizable(win);
    win->show();
    return(fltk::run());
}
