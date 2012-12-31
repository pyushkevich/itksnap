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
#include <SnakeToolROIPanel.h>
#include <DisplayLayoutInspector.h>

#include <SnakeWizardPanel.h>

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
#include "QtWidgetActivator.h"

const char *IRISMainToolbox::m_InspectorPageNames[] = {
    "Cursor Inspector", "Zoom Inspector", "Label Inspector",
    "Display Inspector", "Tool Inspector"
};


// TODO: This is inconsistent with the rest of the GUI because the widgets
// are constructed in C++ code, not in the UI designer. We should redo this
// using the designer.
IRISMainToolbox::IRISMainToolbox(QWidget *parent) :
    SNAPComponent(parent)
{
  m_Model = NULL;

  // Start from scratch
  ApplyCSS(this, ":/root/itksnap.css");

  // Box layout to stretch everything out
  QVBoxLayout *lMain = new QVBoxLayout(this);
  lMain->setContentsMargins(5,5,5,5);
  lMain->setSpacing(8);

  this->setMaximumWidth(180);

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
    "crosshair.gif", "zoom.gif", "paintbrush.gif", "dl_toolbox.png", "tools.png" };

  m_ZoomInspector = new ZoomInspector(this);
  m_CursorInspector = new CursorInspector(this);
  m_LabelInspector = new LabelInspector(this);
  m_DisplayLayoutInspector = new DisplayLayoutInspector(this);

  // The tools page is just a widget with a stack layout
  m_ToolInspector = new QStackedWidget();

  // Add the various tool panels
  m_SnakeToolROIPanel = new SnakeToolROIPanel();
  m_ToolInspector->addWidget(new QWidget(this));
  m_ToolInspector->addWidget(m_SnakeToolROIPanel);

  QWidget *tabContent[] = {
    m_CursorInspector, m_ZoomInspector, m_LabelInspector,
    m_DisplayLayoutInspector, m_ToolInspector
  };

  // Add the four tabs to the tab bar
  for(int i = 0; i < 5; i++)
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
  // Set the models
  m_Model = model;
  m_ZoomInspector->SetModel(model);
  m_CursorInspector->SetModel(model->GetCursorInspectionModel());
  m_LabelInspector->SetModel(model);
  m_SnakeToolROIPanel->SetModel(model);
  m_DisplayLayoutInspector->SetModel(model->GetDisplayLayoutModel());

  // Set up state machine
  activateOnNotFlag(
        this->findChild<QWidget *>("BtnPolygonMode"), model, UIF_SNAKE_MODE);
  activateOnNotFlag(
        this->findChild<QWidget *>("BtnSnakeMode"), model, UIF_SNAKE_MODE);

  // Listen to changes in the toolbar mode
  connectITK(m_Model->GetToolbarModeModel(), ValueChangedEvent());
}

void IRISMainToolbox::onModelUpdate(const EventBucket &bucket)
{
  // Respond to changes in toolbar mode
  if(bucket.HasEvent(ValueChangedEvent()))
    {
    switch(m_Model->GetToolbarMode())
      {
      case CROSSHAIRS_MODE:
        m_TabInspector->setCurrentIndex(0);
        m_ToolInspector->setCurrentIndex(0);
        break;

      case NAVIGATION_MODE:
        m_TabInspector->setCurrentIndex(1);
        m_ToolInspector->setCurrentIndex(0);
        break;

      case POLYGON_DRAWING_MODE:
        m_TabInspector->setCurrentIndex(2);
        m_ToolInspector->setCurrentIndex(0);
        break;

      case ROI_MODE:
        m_TabInspector->setCurrentIndex(3);
        m_ToolInspector->setCurrentIndex(1);
        break;

      case PAINTBRUSH_MODE:
        break;
      case ANNOTATION_MODE:
        break;
      }
    }
}

void IRISMainToolbox::on_BtnCrosshairMode_toggled(bool checked)
{
  if(checked)
    {
    // Enter crosshair mode
    m_Model->SetToolbarMode(CROSSHAIRS_MODE);
    }
}

void IRISMainToolbox::on_BtnZoomPanMode_toggled(bool checked)
{
  if(checked)
    {
    // Enter crosshair mode
    m_Model->SetToolbarMode(NAVIGATION_MODE);
    }
}

void IRISMainToolbox::on_BtnPolygonMode_toggled(bool checked)
{
  if(checked)
    {
    m_Model->SetToolbarMode(POLYGON_DRAWING_MODE);
    }
}

void IRISMainToolbox::on_BtnSnakeMode_toggled(bool checked)
{
  if(checked)
    {
    m_Model->SetToolbarMode(ROI_MODE);
    }
}

void IRISMainToolbox::on_TabInspector_currentChanged(int index)
{
  m_GroupInspector->setTitle(m_InspectorPageNames[index]);
}


