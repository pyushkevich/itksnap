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
#include "DisplayLayoutModel.h"
#include "ColorMapModel.h"
#include "ViewPanel3D.h"
#include "IRISMainToolbox.h"
#include "SnakeWizardPanel.h"
#include "LatentITKEventNotifier.h"
#include <QProgressDialog>
#include "QtReporterDelegates.h"

#include "QtCursorOverride.h"
#include "QtWarningDialog.h"
#include <QtWidgetCoupling.h>

#include <LabelEditorDialog.h>
#include <ReorientImageDialog.h>
#include <QAbstractListModel>
#include <itksys/SystemTools.hxx>
#include <QItemDelegate>
#include <QPainter>
#include <QDockWidget>
#include <QMessageBox>


/**
  Traits used to display an item from the image history as an entry in
  the table of recently loaded images
  */
class HistoryListModel : public QAbstractListModel
{
public:

  int rowCount(const QModelIndex &parent) const
  {
    SystemInterface::HistoryListType &history =
        m_Model->GetDriver()->GetSystemInterface()->GetHistory(m_HistoryName.c_str());

    // Display at most 12 entries in the history
    return std::min((size_t) 12, history.size());
  }

  QVariant data(const QModelIndex &index, int role) const
  {
    // Get the history
    SystemInterface::HistoryListType &history =
        m_Model->GetDriver()->GetSystemInterface()->GetHistory(m_HistoryName.c_str());

    // Get the entry
    std::string item = history[history.size() - (1 + index.row())];

    // Display the appropriate item
    if(role == Qt::DisplayRole)
      {
      // Get the shorter filename
      std::string shorty = itksys::SystemTools::GetFilenameName(item.c_str());
      return QString(shorty.c_str());
      }
    else if(role == Qt::DecorationRole)
      {
      // Need to get an icon!
      std::string iconfile = m_Model->GetDriver()->GetSystemInterface()
          ->GetThumbnailAssociatedWithFile(item.c_str());

      // Need to load the icon
      QIcon icon;
      icon.addFile(iconfile.c_str());      
      return icon;
      }
    else if(role == Qt::ToolTipRole)
      {
      return QString(item.c_str());
      }
    return QVariant();
  }

  irisGetSetMacro(Model, GlobalUIModel *)

  irisGetSetMacro(HistoryName, std::string)

  HistoryListModel() { m_Model = NULL; }

public slots:

  void onModelUpdate(EventBucket &bucket)
  {
    if(bucket.HasEvent(MainImageDimensionsChangeEvent()))
      {
      this->reset();
      }
  }

protected:
  // Need a pointer to the model
  GlobalUIModel *m_Model;

  // The name of the history
  std::string m_HistoryName;
};

class HistoryListItemDelegate : public QItemDelegate
{

public:
  HistoryListItemDelegate(QWidget *parent) : QItemDelegate(parent) {}

  void paint(QPainter *painter,
             const QStyleOptionViewItem &option,
             const QModelIndex &index) const
  {
    if(!(option.state & QStyle::State_MouseOver))
      {
      painter->setOpacity(0.8);
      }
    QItemDelegate::paint(painter, option, index);
  }
};


MainImageWindow::MainImageWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainImageWindow)
{
  ui->setupUi(this);

  // Create the view panels array
  m_ViewPanels[0] = ui->panel0;
  m_ViewPanels[1] = ui->panel1;
  m_ViewPanels[2] = ui->panel2;
  m_ViewPanels[3] = ui->panel3D;

  // Initialize the dialogs
  m_LabelEditor = new LabelEditorDialog(this);
  m_LabelEditor->setModal(false);

  m_LayerInspector = new LayerInspectorDialog(this);
  m_LayerInspector->setModal(false);

  m_ReorientImageDialog = new ReorientImageDialog(this);
  m_ReorientImageDialog->setModal(false);

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

  // Delegate for history
  HistoryListItemDelegate *del = new HistoryListItemDelegate(ui->listRecent);
  ui->listRecent->setItemDelegate(del);

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

  // Hook up buttons to actions
  connect(ui->btnLoadGrey, SIGNAL(clicked()), ui->actionOpenGrey, SLOT(trigger()));
  connect(ui->btnLoadRGB, SIGNAL(clicked()), ui->actionOpenRGB, SLOT(trigger()));

  // Set up the progress dialog
  m_Progress = new QProgressDialog(this);

  // Create the delegate to pass in to the model
  m_ProgressReporterDelegate = new QtProgressReporterDelegate();
  m_ProgressReporterDelegate->SetProgressDialog(m_Progress);
}

MainImageWindow::~MainImageWindow()
{
  delete m_ProgressReporterDelegate;
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
  m_ReorientImageDialog->SetModel(model->GetReorientImageModel());

  // Initialize the docked panels
  m_Toolbox->SetModel(model);

  // Attach the progress reporter delegate to the model
  m_Model->SetProgressReporterDelegate(m_ProgressReporterDelegate);

  // Listen for changes to the main image, updating the recent image file
  // menu. TODO: a more direct way would be to listen to changes to the
  // history, but that requires making history an event-firing object
  LatentITKEventNotifier::connect(model->GetDriver(),
                                  MainImageDimensionsChangeEvent(),
                                  this,
                                  SLOT(onModelUpdate(EventBucket)));

  // Create a model for the table of recent images and connect to the widget
  HistoryListModel *historyModel = new HistoryListModel();
  historyModel->SetModel(m_Model);
  historyModel->SetHistoryName("MainImage");
  ui->listRecent->setModel(historyModel);

  // Make the model listen to events affecting history
  LatentITKEventNotifier::connect(model->GetDriver(),
                                  MainImageDimensionsChangeEvent(),
                                  historyModel,
                                  SLOT(onModelUpdate(EventBucket)));

  // Couple the visibility of each view panel to the correponding property
  // model in DisplayLayoutModel
  DisplayLayoutModel *layoutModel = m_Model->GetDisplayLayoutModel();
  for(int i = 0; i < 4; i++)
    {
    makeWidgetVisibilityCoupling(m_ViewPanels[i],
                                 layoutModel->GetViewPanelVisibilityModel(i));
    }

  // Populate the recent file menu
  this->UpdateRecentMenu();
}


// Slot for model updates
void MainImageWindow::onModelUpdate(const EventBucket &b)
{
  if(b.HasEvent(MainImageDimensionsChangeEvent()))
    {
    // Update the recent items
    this->UpdateRecentMenu();

    // Choose what page to show depending on if an image has been loaded
    if(m_Model->GetDriver()->IsMainImageLoaded())
      ui->stackMain->setCurrentWidget(ui->pageMain);
    else
      ui->stackMain->setCurrentWidget(ui->pageSplash);
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


void MainImageWindow::LoadRecent(QString file)
{
  // TODO: prompt for changes!

  // Try loading the image
  try
    {
    // Change cursor for this operation
    QtCursorOverride c(Qt::WaitCursor);
    IRISWarningList warnings;
    LoadMainImageDelegate del(m_Model, IRISApplication::MAIN_ANY);
    m_Model->LoadImageNonInteractive(file.toAscii(), del, warnings);
    }
  catch(exception &exc)
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

void MainImageWindow::on_listRecent_clicked(const QModelIndex &index)
{
  // Load the appropriate image
  QVariant filename = ui->listRecent->model()->data(index, Qt::ToolTipRole);
  this->LoadRecent(filename.toString());
}

void MainImageWindow::on_actionUnload_All_triggered()
{
  // Unload the main image
  m_Model->GetDriver()->UnloadMainImage();
}

void MainImageWindow::on_actionReorient_Image_triggered()
{
  // Show the reorientation dialog
  m_ReorientImageDialog->show();
}
