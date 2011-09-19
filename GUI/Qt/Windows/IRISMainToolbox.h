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

#ifndef IRISMAINTOOLBOX_H
#define IRISMAINTOOLBOX_H

#include <QWidget>
#include <SNAPCommon.h>

class QToolBox;
class QFrame;
class QToolButton;
class GlobalUIModel;
class ZoomInspector;
class QStackedWidget;
class CursorInspector;
class QGroupBox;

class IRISMainToolbox : public QWidget
{
  Q_OBJECT

public:
  explicit IRISMainToolbox(QWidget *parent = 0);
  ~IRISMainToolbox() {}

  irisGetMacro(Model, GlobalUIModel *)

  void SetModel(GlobalUIModel *model);

public slots:

  void on_BtnCrosshairMode_toggled(bool);
  void on_BtnZoomPanMode_toggled(bool);
  void on_TabInspector_currentChanged(int index);

private:

  // The Global UI model with which to interact
  GlobalUIModel *m_Model;

  // Group box that describes the current inspector
  QGroupBox *m_GroupInspector;

  // Names of the inspector pages
  static const char *m_InspectorPageNames[];

  // Toolbox for zoom/pan actions
  ZoomInspector *m_ZoomInspector;
  CursorInspector *m_CursorInspector;
};

#endif // IRISMAINTOOLBOX_H
