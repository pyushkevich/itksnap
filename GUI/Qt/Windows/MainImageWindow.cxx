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

#include "MainControlPanel.h"
#include "ImageIOWizard.h"
#include "ImageIOWizardModel.h"
#include "GlobalUIModel.h"
#include "ImageIODelegates.h"
#include "LayerInspectorDialog.h"
#include "IntensityCurveModel.h"
#include "DisplayLayoutModel.h"
#include "ColorMapModel.h"
#include "ViewPanel3D.h"
#include "SnakeWizardPanel.h"
#include "LatentITKEventNotifier.h"
#include <QProgressDialog>
#include "QtReporterDelegates.h"
#include "SliceWindowCoordinator.h"
#include "HistoryQListModel.h"
#include "GenericView3D.h"
#include "GenericSliceView.h"
#include "SplashPanel.h"
#include "QtWidgetCoupling.h"
#include "SimpleFileDialogWithHistory.h"
#include "StatisticsDialog.h"
#include "ImageWrapperBase.h"
#include "IRISImageData.h"

#include "QtCursorOverride.h"
#include "QtWarningDialog.h"
#include <QtWidgetCoupling.h>
#include <QtWidgetActivator.h>
#include <QtActionGroupCoupling.h>

#include <LabelEditorDialog.h>
#include <ReorientImageDialog.h>
#include <DropActionDialog.h>


#include <QAbstractListModel>
#include <QItemDelegate>
#include <QPainter>
#include <QDockWidget>
#include <QMessageBox>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QUrl>
#include <QFileDialog>
#include <QGLWidget>
#include <SNAPQtCommon.h>


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

  // Group mutually exclusive actions into action groups
  QActionGroup *grpToolbarMain = new QActionGroup(this);
  ui->actionCrosshair->setActionGroup(grpToolbarMain);
  ui->actionZoomPan->setActionGroup(grpToolbarMain);
  ui->actionPolygon->setActionGroup(grpToolbarMain);
  ui->actionPaintbrush->setActionGroup(grpToolbarMain);
  ui->actionSnake->setActionGroup(grpToolbarMain);

  QActionGroup *grpToolbar3D = new QActionGroup(this);
  ui->action3DTrackball->setActionGroup(grpToolbar3D);
  ui->action3DCrosshair->setActionGroup(grpToolbar3D);
  ui->action3DSpray->setActionGroup(grpToolbar3D);
  ui->action3DScalpel->setActionGroup(grpToolbar3D);

  // Make sure we initialize on the intro page
  ui->stackMain->setCurrentWidget(ui->pageSplash);

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

  m_DropDialog = new DropActionDialog(this);

  m_StatisticsDialog = new StatisticsDialog(this);
  m_StatisticsDialog->setModal(false);

  // Initialize the docked panels
  m_DockLeft = new QDockWidget(this);
  m_DockLeft->setAllowedAreas(Qt::LeftDockWidgetArea);
  m_DockLeft->setFeatures(
        QDockWidget::DockWidgetFloatable |
        QDockWidget::DockWidgetMovable);
  m_DockLeft->setWindowTitle("ITK-SNAP Toolbox");
  this->addDockWidget(Qt::LeftDockWidgetArea, m_DockLeft);

  m_ControlPanel = new MainControlPanel(this);
  m_DockLeft->setWidget(m_ControlPanel);

  m_DockRight = new QDockWidget("Segment 3D", this);
  m_SnakeWizard = new SnakeWizardPanel(this);
  m_DockRight->setWidget(m_SnakeWizard);
  m_DockRight->setAllowedAreas(Qt::LeftDockWidgetArea);
  this->addDockWidget(Qt::RightDockWidgetArea, m_DockRight);

  // Delegate for history
  HistoryListItemDelegate *del = new HistoryListItemDelegate(ui->listRecent);
  ui->listRecent->setItemDelegate(del);

  // Set the splash panel in the left dock
  m_SplashPanel = new SplashPanel(this);
  m_DockLeft->setWidget(m_SplashPanel);

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
  connect(ui->btnLoadMain, SIGNAL(clicked()), ui->actionOpenMain, SLOT(trigger()));

  // Add actions that are not on the menu
  addAction(ui->actionZoomToFitInAllViews);
  addAction(ui->actionCenter_on_Cursor);

  // Set up the progress dialog
  m_Progress = new QProgressDialog(this);

  // Create the delegate to pass in to the model
  m_ProgressReporterDelegate = new QtProgressReporterDelegate();
  m_ProgressReporterDelegate->SetProgressDialog(m_Progress);

  // Set title
  this->setWindowTitle("ITK-SNAP");

  // We accept drop events
  setAcceptDrops(true);
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
  m_DropDialog->SetModel(model);
  m_StatisticsDialog->SetModel(model);

  // Initialize the docked panels
  m_ControlPanel->SetModel(model);

  // Attach the progress reporter delegate to the model
  m_Model->SetProgressReporterDelegate(m_ProgressReporterDelegate);

  // Listen for changes to the main image, updating the recent image file
  // menu. TODO: a more direct way would be to listen to changes to the
  // history, but that requires making history an event-firing object
  LatentITKEventNotifier::connect(model->GetDriver(),
                                  LayerChangeEvent(),
                                  this,
                                  SLOT(onModelUpdate(const EventBucket&)));

  // Create a model for the table of recent images and connect to the widget
  HistoryQListModel *historyModel = new HistoryQListModel();
  historyModel->SetModel(m_Model);
  historyModel->SetHistoryName("MainImage");
  ui->listRecent->setModel(historyModel);

  // Make the model listen to events affecting history
  LatentITKEventNotifier::connect(model->GetDriver(),
                                  MainImageDimensionsChangeEvent(),
                                  historyModel,
                                  SLOT(onModelUpdate(const EventBucket&)));

  // Listen to metadata changes, since they affect the title window
  LatentITKEventNotifier::connect(model->GetDriver(),
                                  WrapperMetadataChangeEvent(),
                                  this,
                                  SLOT(onModelUpdate(const EventBucket&)));

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

  // Set up activations
  activateOnFlag(ui->actionUndo, m_Model, UIF_UNDO_POSSIBLE);
  activateOnFlag(ui->actionRedo, m_Model, UIF_REDO_POSSIBLE);

  // Tool action activations
  activateOnFlag(ui->actionCrosshair, m_Model, UIF_BASEIMG_LOADED);
  activateOnFlag(ui->actionZoomPan, m_Model, UIF_BASEIMG_LOADED);
  activateOnFlag(ui->actionPolygon, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);
  activateOnFlag(ui->actionSnake, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);
  activateOnFlag(ui->actionPaintbrush, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);

  activateOnFlag(ui->action3DCrosshair, m_Model, UIF_BASEIMG_LOADED);
  activateOnFlag(ui->action3DTrackball, m_Model, UIF_BASEIMG_LOADED);
  activateOnFlag(ui->action3DScalpel, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);
  activateOnFlag(ui->action3DSpray, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);

  // Hook up toolbar actions to the toolbar
  makeActionGroupCoupling(this->GetMainToolActionGroup(),
                          m_Model->GetToolbarModeModel());

  makeActionGroupCoupling(this->Get3DToolActionGroup(),
                          m_Model->GetToolbarMode3DModel());


}

void MainImageWindow::ShowFirstTime()
{
  // Before showing the window, select the right pages for the main view
  // and the sidebar, otherwise we get an annoying flashing of windows at
  // startup, as the pages are changed in response to an event
  this->UpdateMainLayout();

  // Also make sure the other elements look right before showing the window
  this->UpdateRecentMenu();
  this->UpdateWindowTitle();

  // Show the window
  this->show();
}

// Slot for model updates
void MainImageWindow::onModelUpdate(const EventBucket &b)
{
  if(b.HasEvent(MainImageDimensionsChangeEvent()))
    {
    // Update the recent items
    this->UpdateRecentMenu();
    this->UpdateMainLayout();
    }

  if(b.HasEvent(LayerChangeEvent()) || b.HasEvent(WrapperMetadataChangeEvent()))
    {
    // Update the window title
    this->UpdateWindowTitle();
    }
}

void MainImageWindow::UpdateMainLayout()
{
  // Choose what page to show depending on if an image has been loaded
  if(m_Model->GetDriver()->IsMainImageLoaded())
    {
    ui->stackMain->setCurrentWidget(ui->pageMain);
    m_DockLeft->setWidget(m_ControlPanel);
    }
  else
    {
    ui->stackMain->setCurrentWidget(ui->pageSplash);
    m_DockLeft->setWidget(m_SplashPanel);
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
      menus[i]->setText(from_utf8(recent[i]));
      menus[i]->setEnabled(true);
      }
    else
      {
      menus[i]->setText("Not available");
      menus[i]->setEnabled(false);
      }
    }
}

void MainImageWindow::UpdateWindowTitle()
{
  GenericImageData *gid = m_Model->GetDriver()->GetIRISImageData();
  QString mainfile, segfile;
  if(gid && gid->IsMainLoaded())
    {
    mainfile = from_utf8(gid->GetMain()->GetNickname());
    segfile = from_utf8(gid->GetSegmentation()->GetNickname());
    }

  if(mainfile.length())
    {
    if(segfile.length())
      {
      this->setWindowTitle(QString("%1 - %2 - ITK-SNAP").arg(mainfile).arg(segfile));
      ui->actionSaveSegmentation->setText(QString("Save \"%1\"").arg(segfile));
      ui->actionSaveSegmentationAs->setText(QString("Save \"%1\" as...").arg(segfile));
      }
    else
      {
      this->setWindowTitle(QString("%1 - New Segmentation - ITK-SNAP").arg(mainfile));
      ui->actionSaveSegmentation->setText(QString("Save"));
      ui->actionSaveSegmentationAs->setText(QString("Save as..."));

      }
    }
  else
    {
    this->setWindowTitle("ITK-SNAP");
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

void MainImageWindow::on_actionQuit_triggered()
{ 
  // TODO: check for unsaved changes

  // Close all the windows that are open
  QApplication::closeAllWindows();

  // Unload all images (this causes the associations to be saved)
  m_Model->GetDriver()->Quit();

  // Exit the application
  QCoreApplication::exit();
}

void MainImageWindow::on_actionLoad_from_Image_triggered()
{
  // TODO: Prompt for changes to segmentation to be saved

  // Create a model for IO
  LoadSegmentationImageDelegate delegate(m_Model);
  SmartPtr<ImageIOWizardModel> model = ImageIOWizardModel::New();
  model->InitializeForLoad(m_Model, &delegate,
                           "LabelImage", "Segmentation Image");

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

void MainImageWindow::dragEnterEvent(QDragEnterEvent *event)
{
  const QMimeData *md = event->mimeData();
  if(md->hasUrls() && md->urls().size() == 1)
    {
    QUrl url = md->urls().first();
    if(url.isLocalFile())
      {
      event->setDropAction(Qt::CopyAction);
      event->accept();
      }
    }
}

void MainImageWindow::LoadDroppedFile(QString file)
{
  if(m_Model->GetDriver()->IsMainImageLoaded())
    {
    // If an image is already loaded, we show the dialog
    m_DropDialog->SetDroppedFilename(file);
    m_DropDialog->setModal(true);
    m_DropDialog->show();
    }
  else
    {
    // Otherwise, attempt to load the image
    this->LoadRecent(file);
    }
}

void MainImageWindow::dropEvent(QDropEvent *event)
{
  QString file = event->mimeData()->urls().first().path();
  LoadDroppedFile(file);
  event->acceptProposedAction();
}

QActionGroup *MainImageWindow::GetMainToolActionGroup()
{
  return ui->actionCrosshair->actionGroup();
}

QActionGroup *MainImageWindow::Get3DToolActionGroup()
{
  return ui->action3DCrosshair->actionGroup();
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
    LoadMainImageDelegate del(m_Model);
    m_Model->LoadImageNonInteractive(file.toUtf8().constData(), del, warnings);
    }
  catch(exception &exc)
    {
    ReportNonLethalException(this, exc, "Image IO Error",
                             QString("Failed to load image %1").arg(file));
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

void MainImageWindow::on_actionZoomToFitInAllViews_triggered()
{
  // Reset the common zoom factor
  m_Model->GetSliceCoordinator()->ResetViewToFitInAllWindows();
}

void MainImageWindow::on_actionCenter_on_Cursor_triggered()
{
  m_Model->GetSliceCoordinator()->CenterViewOnCursorInAllWindows();
}

void MainImageWindow::on_actionUndo_triggered()
{
  m_Model->GetDriver()->Undo();
}

void MainImageWindow::on_actionRedo_triggered()
{
  m_Model->GetDriver()->Redo();
}

/** Filter application-level events */
bool MainImageWindow::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::FileOpen)
    {
    QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(event);
    LoadDroppedFile(openEvent->url().path());
    return true;
    }
  else return false;
}

void MainImageWindow::on_actionOpenMain_triggered()
{
  // TODO: Prompt for changes to segmentation to be saved

  // Create a model for IO
  LoadMainImageDelegate delegate(m_Model);
  SmartPtr<ImageIOWizardModel> model = ImageIOWizardModel::New();
  model->InitializeForLoad(m_Model, &delegate,
                           "AnatomicImage", "Main Image");

  // Execute the IO wizard
  ImageIOWizard wiz(this);
  wiz.SetModel(model);
  wiz.exec();

}

void MainImageWindow::on_actionAdd_Overlay_triggered()
{
  LoadOverlayImageDelegate delegate(m_Model);
  SmartPtr<ImageIOWizardModel> model = ImageIOWizardModel::New();
  model->InitializeForLoad(m_Model, &delegate,
                           "AnatomicImage", "Overlay Image");

  // Execute the IO wizard
  ImageIOWizard wiz(this);
  wiz.SetModel(model);
  wiz.exec();
}

void MainImageWindow::ExportScreenshot(int panelIndex)
{
  // Generate a filename for the screenshot
  std::string finput = m_Model->GenerateScreenshotFilename();

  // Open a file browser and have the user select something
  QString fuser = QFileDialog::getSaveFileName(
        this,
        "Save Snapshot As",
        finput.c_str(),
        "PNG Images (*.png)");

  // If nothing selected, exit
  if(fuser.length() == 0)
    return;

  // What panel is this?
  QtAbstractOpenGLBox *target = NULL;
  if(panelIndex == 3)
    {
    target = ui->panel3D->Get3DView();
    }
  else
    {
    SliceViewPanel *svp = reinterpret_cast<SliceViewPanel *>(m_ViewPanels[panelIndex]);
    target = svp->GetSliceView();
    }

  // Call the screenshot saving method, which will execute asynchronously
  target->SaveScreenshot(to_utf8(fuser));

  // Store the last filename
  m_Model->SetLastScreenshotFileName(to_utf8(fuser));
}

void MainImageWindow::on_actionSSAxial_triggered()
{
  ExportScreenshot(
        m_Model->GetDriver()->GetDisplayWindowForAnatomicalDirection(ANATOMY_AXIAL));
}

void MainImageWindow::on_actionSSCoronal_triggered()
{
  ExportScreenshot(
        m_Model->GetDriver()->GetDisplayWindowForAnatomicalDirection(ANATOMY_CORONAL));
}

void MainImageWindow::on_actionSSSagittal_triggered()
{
  ExportScreenshot(
        m_Model->GetDriver()->GetDisplayWindowForAnatomicalDirection(ANATOMY_SAGITTAL));
}


void MainImageWindow::on_actionSegmentationIncreaseOpacity_triggered()
{
  int opacity = m_Model->GetSegmentationOpacity();
  m_Model->SetSegmentationOpacity(std::min(opacity+5, 100));
}

void MainImageWindow::on_actionSegmentationDecreaseOpacity_triggered()
{
  int opacity = m_Model->GetSegmentationOpacity();
  m_Model->SetSegmentationOpacity(std::max(opacity-5, 0));
}

void MainImageWindow::on_actionSegmentationToggle_triggered()
{
  bool value = m_Model->GetSegmentationVisibility();
  m_Model->SetSegmentationVisibility(!value);
}


void MainImageWindow::on_actionLoadLabels_triggered()
{
  // Ask for a filename
  QString selection = ShowSimpleOpenDialogWithHistory(
        m_Model, "LabelDescriptions",
        "Open Label Descriptions - ITK-SNAP",
        "Label Description File",
        "Text Files (*.txt);; Label Files (*.label);; All Files (*)");

  // Open the labels from the selection
  if(selection.length())
    {
    try
      {
      std::string utf = to_utf8(selection);
      m_Model->GetDriver()->LoadLabelDescriptions(utf.c_str());
      }
    catch(std::exception &exc)
      {
      ReportNonLethalException(this, exc, "Label Description IO Error",
                               QString("Failed to load label descriptions"));
      }
    }
}

void MainImageWindow::on_actionSaveLabels_triggered()
{
  // Ask for a filename
  QString selection = ShowSimpleSaveDialogWithHistory(
        m_Model, "LabelDescriptions",
        "Save Label Descriptions - ITK-SNAP",
        "Label Description File",
        "Text Files (*.txt);; Label Files (*.label);; All Files (*)");

  // Open the labels from the selection
  if(selection.length())
    {
    try
      {
      std::string utf = to_utf8(selection);
      m_Model->GetDriver()->SaveLabelDescriptions(utf.c_str());
      }
    catch(std::exception &exc)
      {
      ReportNonLethalException(this, exc, "Label Description IO Error",
                               QString("Failed to save label descriptions"));
      }
    }
}

void MainImageWindow::on_actionVolumesAndStatistics_triggered()
{
  m_StatisticsDialog->Activate();
}

void MainImageWindow::SaveSegmentation(bool interactive)
{
  // Create delegate
  DefaultSaveImageDelegate delegate(
        m_Model, m_Model->GetDriver()->GetCurrentImageData()->GetSegmentation(),
        "LabelImage");

  // Create a model for IO
  SmartPtr<ImageIOWizardModel> model = ImageIOWizardModel::New();
  model->InitializeForSave(m_Model, &delegate,
                           "LabelImage",
                           "Segmentation Image");

  // Interactive or not?
  if(interactive || model->GetSuggestedFilename().size() == 0)
    {
    // Execute the IO wizard
    ImageIOWizard wiz(this);
    wiz.SetModel(model);
    wiz.exec();
    }
  else
    {
    try
      {
      model->SaveImage(model->GetSuggestedFilename());
      }
    catch(std::exception &exc)
      {
      ReportNonLethalException(
            this, exc, "Image IO Error",
            QString("Failed to save image %1").arg(
              from_utf8(model->GetSuggestedFilename())));
      }
    }
}

void MainImageWindow::on_actionSaveSegmentation_triggered()
{
  SaveSegmentation(false);
}

void MainImageWindow::on_actionSaveSegmentationAs_triggered()
{
  SaveSegmentation(true);
}

