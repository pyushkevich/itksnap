/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: PopupButtonInteractionMode.cxx,v $
  Language:  C++
  Date:      $Date: 2010/10/12 16:02:05 $
  Version:   $Revision: 1.2 $
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
#include "PopupButtonInteractionMode.h"
#include "SNAPOpenGL.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "IRISImageData.h"
#include "SNAPAppearanceSettings.h"
#include "UserInterfaceBase.h"
#include "SliceWindowCoordinator.h"
#include "UserInterfaceLogic.h"

PopupButtonInteractionMode
::PopupButtonInteractionMode(GenericSliceWindow *parent)
: GenericSliceWindow::EventHandler(parent)
{
}

#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Menu_Bar.H>

int 
PopupButtonInteractionMode
::OnMousePress (const FLTKEvent &event)
{
  // Only draw when UI is not visible
  DisplayLayout dl = m_ParentUI->GetDisplayLayout();
  if(dl.show_main_ui)
    return 0;
  
  // Get the coordiantes of the mouse click
  int w = this->GetCanvas()->w();
  int h = this->GetCanvas()->h();
  int x = event.XCanvas[0], y = event.XCanvas[1];

  if(x <= w-5 && x >= w-15 && y <= h-5 && y >= h-15 
    && (x - (w-15)) > ((h-5) - y))
    {
    // Dynamically create menu, pop it up
    Fl_Menu_Button menu(Fl::event_x_root(), Fl::event_y_root(), 80, 1);
    Fl_Menu_Bar *bar = m_ParentUI->GetMainMenuBar();
    menu.copy(bar->menu());

    // Put the menu so that it's as deep in the widget hierarchy as the main menubar
    Fl_Group g1(Fl::event_x_root(), Fl::event_y_root(), 80, 1);
    Fl_Group g2(Fl::event_x_root(), Fl::event_y_root(), 80, 1);
    m_ParentUI->GetMainWindow()->add(g1);
    g1.add(g2);
    g2.add(menu);

    // Popup
    menu.popup();
    m_ParentUI->GetMainWindow()->remove(g1);
    g1.remove(g2);
    g2.remove(menu);

    // Eat up the event
    return 1;
    }

  return 0;
}

void
PopupButtonInteractionMode
::OnDraw()
{
  // Only draw when UI is not visible
  DisplayLayout dl = m_ParentUI->GetDisplayLayout();
  if(dl.show_main_ui)
    return;
  
  // Set line properties
  glPushAttrib(GL_LIGHTING_BIT | GL_COLOR_BUFFER_BIT);

  glColor3d(0.0, 0.75, 0.0);

  int w = this->GetCanvas()->w();
  int h = this->GetCanvas()->h();

  glDisable(GL_LIGHTING);
  glBegin(GL_TRIANGLES);
  glVertex2f(w-5,  h-5);
  glVertex2f(w-5,  h-15);
  glVertex2f(w-15, h-5);
  glEnd();

  glPopAttrib();
}
