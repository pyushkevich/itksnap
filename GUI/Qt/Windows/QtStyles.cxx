/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: Filename.cxx,v $
  Language:  C++
  Date:      $Date: 2010/10/18 11:25:44 $
  Version:   $Revision: 1.12 $
  Copyright (c) 2011 Paul A. Yushkevich

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

=========================================================================*/

#include "QtStyles.h"
#include <QWidget>
#include <QFile>
#include <iostream>

void ApplyCSS(QWidget *widget, const char *file)
{
  QFile qf(file);
  if(qf.open(QFile::ReadOnly))
    {
    QString qs(qf.readAll());
    widget->setStyleSheet(qs);
    }
  else
    {
    std::cerr << "Can not read CSS from " << file << std::endl;
    }
}

const char *qstPlastiqueButton =
  "* {"
  "  border-image: url(:/root/fltkbutton.png) repeat;"
  "  border-top-width: 8px;"
  "  border-bottom-width: 8px;"
  "  border-left-width: 3px;"
  "  border-right-width: 3px;"
  "  background-origin: content;"
  "  padding-top: -8px;"
  "  padding-bottom: -8px;"
  "  padding-left: -3px;"
  "  padding-right: -3px;"
  "}"
  "*:pressed, *:checked {"
  "  border-image: url(:/root/fltkbutton_pressed.png) repeat;"
  "}";

const char *qstPlastiquePanel =
  "* {"
  "  border-image: url(:/root/fltkpanel.png) repeat;"
  "  border-top-width: 8px;"
  "  border-bottom-width: 8px;"
  "  border-left-width: 2px;"
  "  border-right-width: 2px;"
  "  background-origin: content;"
  "  padding-top: -3px;"
  "  padding-bottom: -3px;"
  "  padding-left: 3px;"
  "  padding-right: 3px;"
  "}";

const char *qstPlastiqueGroupBox =
"QGroupBox {"
"  border-image: url(:/root/fltkpanel.png) repeat;"
"  border-top-width: 8px;"
"  border-bottom-width: 8px;"
"  border-left-width: 2px;"
"  border-right-width: 2px;"
"  background-origin: content;"
"  padding-top: -3px;"
"  padding-bottom: -3px;"
"  padding-left: 3px;"
"  padding-right: 3px;"
"  margin-top: 15px;"
"  font-weight: bold;"
"}"
"QGroupBox::title {"
"  subcontrol-origin: 	margin;"
"  subcontrol-position: top left; "
"}";


