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

#include "IRISMainToolbox.h"
#include <QDockWidget>
#include <QVBoxLayout>
#include <QFrame>
#include <QGridLayout>
#include <QToolBox>
#include <QPushButton>
#include <QToolButton>

#include <ZoomInspector.h>
#include <CursorInspector.h>
#include <LabelInspector.h>

#include <GlobalUIModel.h>

#include <QtStyles.h>

#include <QStackedWidget>
#include <QStackedLayout>
#include <QGroupBox>

#include <QToolBar>
#include <QAction>

#include <QLabel>
#include <QFrame>
#include <QTabWidget>
#include <QTabBar>
#include <QFile>

#include <QGraphicsDropShadowEffect>

const char *IRISMainToolbox::m_InspectorPageNames[] = {
    "Cursor Inspector", "Zoom Inspector", "Label Inspector",
    "Tool Inspector"
};

IRISMainToolbox::IRISMainToolbox(QWidget *parent) :
    QWidget(parent)
{
  m_Model = NULL;

  // Start from scratch
  ApplyCSS(this, ":/root/itksnap.css");

  // Box layout to stretch everything out
  QVBoxLayout *lMain = new QVBoxLayout(this);
  lMain->setContentsMargins(5,5,5,5);
  lMain->setSpacing(8);

  // Create place for 2D buttons
  QGroupBox *tbxSlice = new QGroupBox("Main Toolbox");
  QGridLayout *lTbxSlice = new QGridLayout(tbxSlice);
  lMain->addWidget(tbxSlice);

  // Layout for the top toolbar
  lTbxSlice->setContentsMargins(3,3,3,3);
  lTbxSlice->setSpacing(5);
  lTbxSlice->setColumnStretch(0,1);
  lTbxSlice->setColumnStretch(4,1);

  // Button names
  const char *btnNames[] = {
    "BtnCrosshairMode", "BtnZoomPanMode", "BtnPolygonMode",
    "BtnSnakeMode", "BtnBrushMode", "BtnAnnotateMode",
    "", ""};

  const char *icons[] = {
    "crosshair", "zoom", "poly",
    "snake", "paintbrush", "", "", ""};

  for(int row = 0, i = 0; row < 2; row++)
    {
    for(int col = 0; col < 4 && row*4+col < 6; col++, i++)
      {
      QToolButton *btn = new QToolButton;
      btn->setObjectName(btnNames[i]);
      btn->setIcon(QIcon(QString(":/root/%1.gif").arg(icons[i])));
      btn->setCheckable(true);
      btn->setAutoExclusive(true);
      btn->setMinimumSize(35, 35);
      btn->setIconSize(QSize(28, 28));
      btn->setStyleSheet(qstPlastiqueButton);
      btn->setChecked(row==0 && col==0);
      lTbxSlice->addWidget(btn, row, col+1);
      }
    }

  /*
  // Add a horizontal line
  QFrame *line = new QFrame();
  line->setGeometry(0,0,100,3);
  line->setFrameShape(QFrame::HLine);
  line->setFrameShadow(QFrame::Sunken);
  lMain->addWidget(line);

  // Add the inspector
  lMain->addWidget(new QLabel("Inspectors"));
  */

  // Add a widget for the inspector stuff
  m_GroupInspector = new QGroupBox("Zoom Inspector");
  QVBoxLayout *lInspect = new QVBoxLayout(m_GroupInspector);
  lInspect->setContentsMargins(0,0,0,0);
  lInspect->setSpacing(0);
  lMain->addWidget(m_GroupInspector);

  // Add a tab bar for the inspector
  m_TabInspector = new QTabWidget();
  m_TabInspector->setObjectName("TabInspector");
  lInspect->addWidget(m_TabInspector);

  const char *tabIcons[] = {
    "crosshair.gif", "zoom.gif", "paintbrush.gif", "tools.png" };

  m_ZoomInspector = new ZoomInspector();
  m_CursorInspector = new CursorInspector();
  m_LabelInspector = new LabelInspector();

  QWidget *tabContent[] = {
    m_CursorInspector, m_ZoomInspector, m_LabelInspector,
    new QWidget(),
    new QWidget(), new QWidget()
  };

  // Add five tabs to the tab bar
  for(int i = 0; i < 4; i++)
    {
    QWidget *w = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(w);
    layout->setContentsMargins(0,10,0,0);
    m_TabInspector->addTab(w,
                         QIcon(QString(":/root/%1").arg(tabIcons[i])),"");

    // QLabel *l = new QLabel(tabNames[i]);
    // layout->addWidget(l);

    QFrame *line = new QFrame();
    line->setGeometry(0,0,100,3);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Plain);
    layout->addWidget(line);

    layout->addWidget(tabContent[i],1);
    }

  QFile qss(":/root/icontabwidget.css");
  qss.open(QFile::ReadOnly);
  QString ss = qss.readAll();
  m_TabInspector->setStyleSheet(ss);

  // Create place for 3D buttons
  QGroupBox *tbx3D = new QGroupBox("3D Toolbox");
  QHBoxLayout *lTbx3D = new QHBoxLayout(tbx3D);
  lTbx3D->setContentsMargins(5,5,5,5);
  lMain->addWidget(tbx3D);


  QMetaObject::connectSlotsByName(this);
}

void IRISMainToolbox::SetModel(GlobalUIModel *model)
{
  m_Model = model;
  m_ZoomInspector->SetModel(model);
  m_CursorInspector->SetModel(model->GetCursorInspectionModel());
  m_LabelInspector->SetModel(model);
}

void IRISMainToolbox::on_BtnCrosshairMode_toggled(bool checked)
{
  if(checked)
    {
    // Enter crosshair mode
    m_Model->SetToolbarMode(CROSSHAIRS_MODE);

    // Also tab over to the cursor inspector
    m_TabInspector->setCurrentWidget(m_CursorInspector);
    }
}

void IRISMainToolbox::on_BtnZoomPanMode_toggled(bool checked)
{
  if(checked)
    {
    // Enter crosshair mode
    m_Model->SetToolbarMode(NAVIGATION_MODE);

    // Also tab over to the cursor inspector
    m_TabInspector->setCurrentWidget(m_LabelInspector);
    }
}

void IRISMainToolbox::on_BtnPolygonMode_toggled(bool checked)
{
  if(checked)
    {
    m_Model->SetToolbarMode(POLYGON_DRAWING_MODE);

    // Also tab over to the cursor inspector
    m_TabInspector->setCurrentWidget(m_CursorInspector);
    }
}

void IRISMainToolbox::on_TabInspector_currentChanged(int index)
{
  m_GroupInspector->setTitle(m_InspectorPageNames[index]);
}


