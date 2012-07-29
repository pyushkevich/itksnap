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
#include "ImageIODelegates.h"
#include "LayerInspectorDialog.h"
#include "IntensityCurveModel.h"
#include "ColorMapModel.h"
#include "ViewPanel3D.h"
#include "IRISMainToolbox.h"
#include "SnakeWizardPanel.h"

#include <LabelEditorDialog.h>

MainImageWindow::MainImageWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainImageWindow)
{
  ui->setupUi(this);

  // Initialize the dialogs
  m_LabelEditor = new LabelEditorDialog(this);
  m_LabelEditor->setModal(false);

  m_LayerInspector = new LayerInspectorDialog(this);
  m_LayerInspector->setModal(false);

  // Initialize the docked panels
  m_DockLeft = new QDockWidget("ITK-SNAP Toolbox", this);
  m_Toolbox = new IRISMainToolbox(this);
  m_DockLeft->setWidget(m_Toolbox);
  this->addDockWidget(Qt::LeftDockWidgetArea, m_DockLeft);

  m_DockRight = new QDockWidget("Segment 3D", this);
  m_SnakeWizard = new SnakeWizardPanel(this);
  m_DockRight->setWidget(m_SnakeWizard);
  this->addDockWidget(Qt::RightDockWidgetArea, m_DockRight);

  // Hide the right dock for now
  m_DockRight->setVisible(false);
}

MainImageWindow::~MainImageWindow()
{
  delete ui;
}

void MainImageWindow::Initialize(GlobalUIModel *model)
{
  m_Model = model;

  // Initialize all the child panels
  ui->panel0->Initialize(model,0);
  ui->panel1->Initialize(model,1);
  ui->panel2->Initialize(model,2);
  ui->panel3D->Initialize(model);

  // Initialize the dialogs
  m_LabelEditor->SetModel(model->GetLabelEditorModel());
  m_LayerInspector->SetModel(model);

  // Initialize the docked panels
  m_Toolbox->SetModel(model);
  // m_SnakeWizard->SetModel(model);
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
  // TODO: Prompt for changes to segmentation to be saved

  // Create a model for IO
  LoadMainImageDelegate delegate(m_Model, IRISApplication::MAIN_SCALAR);
  SmartPtr<ImageIOWizardModel> model = ImageIOWizardModel::New();
  model->InitializeForLoad(m_Model, &delegate, "GreyImage");

  // Execute the IO wizard
  ImageIOWizard wiz(this);
  wiz.SetModel(model);
  wiz.exec();
}

void MainImageWindow::on_actionQuit_triggered()
{
  // TODO: check for unsaved changes

  // Exit the application
  QCoreApplication::exit();
}

void MainImageWindow::on_actionLoad_from_Image_triggered()
{
  // TODO: Prompt for changes to segmentation to be saved

  // Create a model for IO
  LoadSegmentationImageDelegate delegate(m_Model);
  SmartPtr<ImageIOWizardModel> model = ImageIOWizardModel::New();
  model->InitializeForLoad(m_Model, &delegate, "LabelImage");

  // Execute the IO wizard
  ImageIOWizard wiz(this);
  wiz.SetModel(model);
  wiz.exec();
}

void MainImageWindow::on_actionAdd_Greyscale_Overlay_triggered()
{
  LoadOverlayImageDelegate delegate(m_Model, IRISApplication::MAIN_SCALAR);
  SmartPtr<ImageIOWizardModel> model = ImageIOWizardModel::New();
  model->InitializeForLoad(m_Model, &delegate, "GreyImage");

  // Execute the IO wizard
  ImageIOWizard wiz(this);
  wiz.SetModel(model);
  wiz.exec();
}

void MainImageWindow::on_actionOpen_RGB_Image_triggered()
{
  // TODO: Prompt for changes to segmentation to be saved

  // Create a model for IO
  LoadMainImageDelegate delegate(m_Model, IRISApplication::MAIN_RGB);
  SmartPtr<ImageIOWizardModel> model = ImageIOWizardModel::New();
  model->InitializeForLoad(m_Model, &delegate, "RGBImage");

  // Execute the IO wizard
  ImageIOWizard wiz(this);
  wiz.SetModel(model);
  wiz.exec();
}

void MainImageWindow::on_actionAdd_RGB_Overlay_triggered()
{
  LoadOverlayImageDelegate delegate(m_Model, IRISApplication::MAIN_RGB);
  SmartPtr<ImageIOWizardModel> model = ImageIOWizardModel::New();
  model->InitializeForLoad(m_Model, &delegate, "RGBImage");

  // Execute the IO wizard
  ImageIOWizard wiz(this);
  wiz.SetModel(model);
  wiz.exec();
}

void MainImageWindow::on_actionImage_Contrast_triggered()
{
  // Make the main image layer active in the models.
  // TODO: This should happen automatically somehow!
  /*
  GreyImageWrapperBase *giw =
      m_Model->GetDriver()->GetCurrentImageData()->GetGrey();
  m_Model->GetIntensityCurveModel()->SetLayer(giw);
  m_Model->GetColorMapModel()->SetLayer(giw);
  */

  // Show the dialog
  m_LayerInspector->show();
}


void MainImageWindow::on_actionLabel_Editor_triggered()
{
  // Execute the label editor
  m_LabelEditor->show();
}

void MainImageWindow::SetSnakeWizardVisible(bool onoff)
{
  m_DockRight->setVisible(onoff);
}
