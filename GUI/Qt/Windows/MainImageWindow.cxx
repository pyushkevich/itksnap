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

#include "MainImageWindow.h"
#include "ui_MainImageWindow.h"

#include "ImageIOWizard.h"
#include "ImageIOWizardModel.h"
#include "GlobalUIModel.h"

MainImageWindow::MainImageWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainImageWindow)
{

  ui->setupUi(this);
/*
  // Some manual stuff
  QDockWidget *dock = new QDockWidget("Tool Dock", this);

  // A widget to hold the tools
  QFrame *frame = new QFrame(this);

  // Add a horizontal layout to the frame
  frame->setLayout(new QVBoxLayout());

  // Add some children to the frame
  QFrame *toolbar = new QFrame(frame);
  toolbar->setLayout(new QGridLayout());
  frame->layout()->addWidget(toolbar);

  QToolButton *tool1 = new QToolButton();
  QToolButton *tool2 = new QToolButton();
  toolbar->layout()->addWidget(tool1);
  toolbar->layout()->addWidget(tool2);

  QToolBox *qt = new QToolBox(frame);
  qt->layout()->addWidget(qt);

  dock->setWidget(frame);
  */
}

MainImageWindow::~MainImageWindow()
{
  delete ui;
}

SliceViewPanel * MainImageWindow::GetSlicePanel(unsigned int i)
{
  if(i == 0)
    return ui->panel0;
  else if (i == 1)
    return ui->panel1;
  else if (i == 2)
    return ui->panel2;
  else
    return NULL;
}

void MainImageWindow::on_actionOpen_Greyscale_Image_triggered()
{
  ImageIOWizard wiz(this);
  ImageIOWizardModel model(m_Model, ImageIOWizardModel::LOAD, "GreyImage");
  wiz.SetModel(&model);
  wiz.exec();
}

void MainImageWindow::on_actionQuit_triggered()
{
  // TODO: check for unsaved changes

  // Exit the application
  QCoreApplication::exit();
}
