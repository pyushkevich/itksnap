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
#include "LatentITKEventNotifier.h"

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
  m_DockLeft->setAllowedAreas(Qt::LeftDockWidgetArea);
  this->addDockWidget(Qt::LeftDockWidgetArea, m_DockLeft);

  m_DockRight = new QDockWidget("Segment 3D", this);
  m_SnakeWizard = new SnakeWizardPanel(this);
  m_DockRight->setWidget(m_SnakeWizard);
  m_DockRight->setAllowedAreas(Qt::LeftDockWidgetArea);
  this->addDockWidget(Qt::RightDockWidgetArea, m_DockRight);

  // Hide the right dock for now
  m_DockRight->setVisible(false);

  // Hide the dock when the wizard finishes
  connect(m_SnakeWizard, SIGNAL(wizardFinished()),
          this, SLOT(onSnakeWizardFinished()));

  // Make the margins adjust when the docks are attached
  connect(m_DockLeft, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)),
          this, SLOT(AdjustMarginsForDocks()));

  connect(m_DockLeft, SIGNAL(topLevelChanged(bool)),
          this, SLOT(AdjustMarginsForDocks()));

  connect(m_DockLeft, SIGNAL(visibilityChanged(bool)),
          this, SLOT(AdjustMarginsForDocks()));

  connect(m_DockRight, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)),
          this, SLOT(AdjustMarginsForDocks()));

  connect(m_DockRight, SIGNAL(topLevelChanged(bool)),
          this, SLOT(AdjustMarginsForDocks()));

  connect(m_DockRight, SIGNAL(visibilityChanged(bool)),
          this, SLOT(AdjustMarginsForDocks()));

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
  m_SnakeWizard->SetModel(model);

  // Initialize the docked panels
  m_Toolbox->SetModel(model);
  // m_SnakeWizard->SetModel(model);

  // Listen for changes to the main image, updating the recent image file
  // menu. TODO: a more direct way would be to listen to changes to the
  // history, but that requires making history an event-firing object
  LatentITKEventNotifier::connect(model->GetDriver(),
                                  MainImageDimensionsChangeEvent(),
                                  this,
                                  SLOT(onModelUpdate(EventBucket)));

  // Populate the recent file menu
  this->UpdateRecentMenu();
}


// Slot for model updates
void MainImageWindow::onModelUpdate(const EventBucket &b)
{
  if(b.HasEvent(MainImageDimensionsChangeEvent()))
    {
    this->UpdateRecentMenu();
    }
}

void MainImageWindow::UpdateRecentMenu()
{
  // Menus to populate
  QAction *menus[] = {
    ui->actionRecent_1,
    ui->actionRecent_2,
    ui->actionRecent_3,
    ui->actionRecent_4,
    ui->actionRecent_5};

  // List of filenames
  std::vector<std::string> recent = m_Model->GetRecentMainImages(5);

  // Toggle the state of each menu item
  for(int i = 0; i < 5; i++)
    {
    if(i < recent.size())
      {
      menus[i]->setText(recent[i].c_str());
      menus[i]->setEnabled(true);
      }
    else
      {
      menus[i]->setText("Not available");
      menus[i]->setEnabled(false);
      }
    }
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

void MainImageWindow::on_actionOpenGrey_triggered()
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

  // Unload all images (this causes the associations to be saved)
  m_Model->GetDriver()->UnloadOverlays();
  m_Model->GetDriver()->UnloadMainImage();

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
  // Show the dialog
  m_LayerInspector->show();
}


void MainImageWindow::on_actionLabel_Editor_triggered()
{
  // Execute the label editor
  m_LabelEditor->show();
}

void MainImageWindow::OpenSnakeWizard()
{
  // Initialize the snake wizard
  this->m_SnakeWizard->Initialize();

  // Make the dock containing the wizard visible
  m_DockRight->setVisible(true);
}

void MainImageWindow::AdjustMarginsForDocks()
{
  // Get the current margins
  QMargins margin = ui->centralwidget->layout()->contentsMargins();
  QMargins mld = m_DockLeft->widget()->layout()->contentsMargins();
  QMargins mrd = m_DockRight->widget()->layout()->contentsMargins();

  // Whether each of the docks is attached
  bool leftDockAtLeft =
      (dockWidgetArea(m_DockLeft) == Qt::LeftDockWidgetArea &&
       !m_DockLeft->isWindow() &&
       m_DockLeft->isVisible());

  bool rightDockAtRight =
      (dockWidgetArea(m_DockRight) == Qt::RightDockWidgetArea &&
       !m_DockRight->isWindow() &&
       m_DockRight->isVisible());

  margin.setLeft(leftDockAtLeft ? 0 : 4);
  margin.setRight(rightDockAtRight ? 0 : 4);
  ui->centralwidget->layout()->setContentsMargins(margin);

  mld.setRight(leftDockAtLeft ? 0 : 5);
  m_DockLeft->widget()->layout()->setContentsMargins(mld);

  mrd.setLeft(rightDockAtRight ? 0 : 5);
  m_DockRight->widget()->layout()->setContentsMargins(mrd);

}

#include "QtWarningDialog.h"

void MainImageWindow::LoadRecent(QString file)
{
  // TODO: prompt for changes!

  // Try loading the image
  try
    {
    IRISWarningList warnings;
    LoadMainImageDelegate del(m_Model, IRISApplication::MAIN_ANY);
    m_Model->LoadImageNonInteractive(file.toAscii(), del, warnings);
    }
  catch(itk::ExceptionObject &exc)
    {
    QMessageBox b(this);
    b.setText(QString("Failed to load image %1").arg(file));
    b.setDetailedText(exc.what());
    b.setIcon(QMessageBox::Critical);
    b.exec();
    }
}

void MainImageWindow::on_actionRecent_1_triggered()
{
  // Load the recent image
  this->LoadRecent(ui->actionRecent_1->text());
}

void MainImageWindow::on_actionRecent_2_triggered()
{
  // Load the recent image
  this->LoadRecent(ui->actionRecent_2->text());
}

void MainImageWindow::on_actionRecent_3_triggered()
{
  // Load the recent image
  this->LoadRecent(ui->actionRecent_3->text());
}

void MainImageWindow::on_actionRecent_4_triggered()
{
  // Load the recent image
  this->LoadRecent(ui->actionRecent_4->text());
}

void MainImageWindow::on_actionRecent_5_triggered()
{
  // Load the recent image
  this->LoadRecent(ui->actionRecent_5->text());
}

void MainImageWindow::onSnakeWizardFinished()
{
  // Make the dock containing the wizard visible
  m_DockRight->setVisible(false);
}
