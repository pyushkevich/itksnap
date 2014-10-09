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
#include "MeshOptions.h"

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
#include "GlobalWSWizardPanel.h"
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
#include "MeshExportWizard.h"
#include "ImageWrapperBase.h"
#include "IRISImageData.h"
#include "AboutDialog.h"
#include "HistoryManager.h"
#include "DefaultBehaviorSettings.h"



#include "QtCursorOverride.h"
#include "QtWarningDialog.h"
#include <QtWidgetCoupling.h>
#include <QtWidgetActivator.h>
#include <QtActionGroupCoupling.h>

#include <LabelEditorDialog.h>
#include <ReorientImageDialog.h>
#include <DropActionDialog.h>
#include <PreferencesDialog.h>
#include "SaveModifiedLayersDialog.h"


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
#include <QDesktopServices>
#include <SNAPQtCommon.h>



MainImageWindow::MainImageWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainImageWindow),
    m_Model(NULL)
{
  ui->setupUi(this);

  // Group mutually exclusive actions into action groups
  QActionGroup *grpToolbarMain = new QActionGroup(this);
  ui->actionCrosshair->setActionGroup(grpToolbarMain);
  ui->actionZoomPan->setActionGroup(grpToolbarMain);
  ui->actionPolygon->setActionGroup(grpToolbarMain);
  ui->actionPaintbrush->setActionGroup(grpToolbarMain);
  ui->actionSnake->setActionGroup(grpToolbarMain);
  ui->actionJoin->setActionGroup(grpToolbarMain);

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
  m_DockRight->setAllowedAreas(Qt::RightDockWidgetArea);
  m_DockRight->setFeatures(
        QDockWidget::DockWidgetFloatable |
        QDockWidget::DockWidgetMovable);
  this->addDockWidget(Qt::RightDockWidgetArea, m_DockRight);

  m_DockRight2 = new QDockWidget("Global Watershed", this);
  m_GlobalWSWizard = new GlobalWSWizardPanel(this);
  m_DockRight2->setWidget(m_GlobalWSWizard);
  m_DockRight2->setAllowedAreas(Qt::RightDockWidgetArea);
  m_DockRight2->setFeatures(
        QDockWidget::DockWidgetFloatable |
        QDockWidget::DockWidgetMovable);
  this->addDockWidget(Qt::RightDockWidgetArea, m_DockRight2);

  // Set up the recent items panels
  connect(ui->panelRecentImages, SIGNAL(RecentItemSelected(QString)),
          SLOT(LoadMainImage(QString)));

  connect(ui->panelRecentWorkspaces, SIGNAL(RecentItemSelected(QString)),
          SLOT(LoadProject(QString)));

  // Set the splash panel in the left dock
  m_SplashPanel = new SplashPanel(this);
  m_DockLeft->setWidget(m_SplashPanel);

  // Create the about dialog
  m_AboutDialog = new AboutDialog(this);

  // Create the preferences dialog
  m_PreferencesDialog = new PreferencesDialog(this);

  // Hide the right dock for now
  m_DockRight->setVisible(false);
  m_DockRight2->setVisible(false);

  // Hide the dock when the wizard finishes
  connect(m_SnakeWizard, SIGNAL(wizardFinished()),
          this, SLOT(onSnakeWizardFinished()));

  // Hide the dock when the wizard finishes
  connect(m_GlobalWSWizard, SIGNAL(wizardFinished()),
          this, SLOT(onGlobalWSWizardFinished()));

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
  connect(ui->btnLoadWorkspace, SIGNAL(clicked()), ui->actionOpenWorkspace, SLOT(trigger()));


  // Set up the progress dialog
  m_Progress = new QProgressDialog(this);

  // Create the delegate to pass in to the model
  m_ProgressReporterDelegate = new QtProgressReporterDelegate();
  m_ProgressReporterDelegate->SetProgressDialog(m_Progress);

  // Set title
  this->setWindowTitle("ITK-SNAP");

  // We accept drop events
  setAcceptDrops(true);

  // Connect recent file/project menu items
  connect(ui->actionRecent_1, SIGNAL(triggered()), SLOT(LoadRecentActionTriggered()));
  connect(ui->actionRecent_2, SIGNAL(triggered()), SLOT(LoadRecentActionTriggered()));
  connect(ui->actionRecent_3, SIGNAL(triggered()), SLOT(LoadRecentActionTriggered()));
  connect(ui->actionRecent_4, SIGNAL(triggered()), SLOT(LoadRecentActionTriggered()));
  connect(ui->actionRecent_5, SIGNAL(triggered()), SLOT(LoadRecentActionTriggered()));
  connect(ui->actionRecentWorkspace1, SIGNAL(triggered()), SLOT(LoadRecentProjectActionTriggered()));
  connect(ui->actionRecentWorkspace2, SIGNAL(triggered()), SLOT(LoadRecentProjectActionTriggered()));
  connect(ui->actionRecentWorkspace3, SIGNAL(triggered()), SLOT(LoadRecentProjectActionTriggered()));
  connect(ui->actionRecentWorkspace4, SIGNAL(triggered()), SLOT(LoadRecentProjectActionTriggered()));
  connect(ui->actionRecentWorkspace5, SIGNAL(triggered()), SLOT(LoadRecentProjectActionTriggered()));

  // Set up the animation timer
  m_AnimateTimer = new QTimer(this);
  m_AnimateTimer->setInterval(1000);
  connect(m_AnimateTimer, SIGNAL(timeout()), SLOT(onAnimationTimeout()));

  // Start the timer (it doesn't cost much...)
  m_AnimateTimer->start();
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
  m_GlobalWSWizard->SetModel(model);
  m_ReorientImageDialog->SetModel(model->GetReorientImageModel());
  m_DropDialog->SetModel(model);
  m_StatisticsDialog->SetModel(model);
  m_PreferencesDialog->SetModel(model->GetGlobalPreferencesModel());

  // Initialize the docked panels
  m_ControlPanel->SetModel(model);

  // Attach the progress reporter delegate to the model
  m_Model->SetProgressReporterDelegate(m_ProgressReporterDelegate);

  // Listen for changes to the main image, updating the recent image file
  // menu. TODO: a more direct way would be to listen to changes to the
  // history, but that requires making history an event-firing object
  LatentITKEventNotifier::connect(model->GetDriver(), LayerChangeEvent(),
                                  this, SLOT(onModelUpdate(const EventBucket&)));

  // Also listen to changes in the image filenames
  LatentITKEventNotifier::connect(model->GetDriver(), WrapperMetadataChangeEvent(),
                                  this, SLOT(onModelUpdate(const EventBucket&)));

  // Hook up the recent lists
  ui->panelRecentImages->Initialize(m_Model, "MainImage");
  ui->panelRecentWorkspaces->Initialize(m_Model, "Project");

  // Observe changes to the main image history, since that affects the recent
  // menu items
  LatentITKEventNotifier::connect(
        model->GetHistoryModel("MainImage"), ValueChangedEvent(),
        this, SLOT(onModelUpdate(EventBucket)));

  // Likewise for the project history, they affect the menu titles
  LatentITKEventNotifier::connect(
        model->GetHistoryModel("Project"), ValueChangedEvent(),
        this, SLOT(onModelUpdate(EventBucket)));

  // Also listen to changes to the project filename
  LatentITKEventNotifier::connect(
        model->GetGlobalState()->GetProjectFilenameModel(), ValueChangedEvent(),
        this, SLOT(onModelUpdate(EventBucket)));

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
  this->UpdateRecentProjectsMenu();

  // Update which page is shown
  this->UpdateMainLayout();

  // Set up activations - File menu
  activateOnFlag(ui->actionOpenMain, m_Model, UIF_IRIS_MODE);
  activateOnFlag(ui->menuRecent_Images, m_Model, UIF_IRIS_MODE);
  activateOnFlag(ui->actionSaveMain, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);
  activateOnFlag(ui->actionSaveSpeed, m_Model, UIF_SNAKE_MODE);
  activateOnFlag(ui->actionSaveLevelSet, m_Model, UIF_LEVEL_SET_ACTIVE);
  activateOnFlag(ui->actionSaveMainROI, m_Model, UIF_SNAKE_MODE);
  activateOnFlag(ui->menuExport, m_Model, UIF_BASEIMG_LOADED);
  activateOnFlag(ui->actionUnload_All, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);

  // Set up activations - Edit menu
  activateOnFlag(ui->actionUndo, m_Model, UIF_UNDO_POSSIBLE);
  activateOnFlag(ui->actionRedo, m_Model, UIF_REDO_POSSIBLE);
  activateOnFlag(ui->actionForegroundLabelNext, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);
  activateOnFlag(ui->actionForegroundLabelPrev, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);
  activateOnFlag(ui->actionBackgroundLabelNext, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);
  activateOnFlag(ui->actionBackgroundLabelPrev, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);

  // Add actions that are not on the menu
  activateOnFlag(ui->actionZoomToFitInAllViews, m_Model, UIF_BASEIMG_LOADED);
  activateOnFlag(ui->actionCenter_on_Cursor, m_Model, UIF_BASEIMG_LOADED);
  activateOnFlag(ui->actionZoom_to_100, m_Model, UIF_BASEIMG_LOADED);
  activateOnFlag(ui->actionZoom_to_200, m_Model, UIF_BASEIMG_LOADED);
  activateOnFlag(ui->actionZoom_to_400, m_Model, UIF_BASEIMG_LOADED);


  // Set up activations - Segmentation menu
  activateOnFlag(ui->actionLoad_from_Image, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);
  activateOnFlag(ui->actionClear, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);
  activateOnFlag(ui->actionSaveSegmentation, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);
  activateOnFlag(ui->actionSaveSegmentationAs, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);
  activateOnFlag(ui->actionSave_as_Mesh, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);
  activateOnFlag(ui->actionLoadLabels, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);
  activateOnFlag(ui->actionSaveLabels, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);
  activateOnFlag(ui->actionVolumesAndStatistics, m_Model, UIF_BASEIMG_LOADED);
  activateOnFlag(ui->menuAppearance, m_Model, UIF_BASEIMG_LOADED);

  // Overlay action activations
  activateOnFlag(ui->actionAdd_Overlay, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);
  activateOnFlag(ui->actionUnload_Last_Overlay, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);
  activateOnFlag(ui->actionUnload_All_Overlays, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);
  activateOnFlag(ui->menuOverlayAppearance, m_Model, UIF_OVERLAY_LOADED);
  activateOnFlag(ui->actionToggleJsrcVis, m_Model, UIF_JOIN_MODE);

  // Workspace menu
  activateOnFlag(ui->actionOpenWorkspace, m_Model, UIF_IRIS_MODE);
  activateOnFlag(ui->actionSaveWorkspace, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);
  activateOnFlag(ui->actionSaveWorkspaceAs, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);

  // Tool action activations
  activateOnFlag(ui->actionCrosshair, m_Model, UIF_NOT_SNAKE_OR_JOIN_MODE);
  activateOnFlag(ui->actionZoomPan, m_Model, UIF_NOT_SNAKE_OR_JOIN_MODE);
  activateOnFlag(ui->actionPolygon, m_Model, UIF_NOT_SNAKE_OR_JOIN_MODE);
  activateOnFlag(ui->actionSnake, m_Model, UIF_NOT_SNAKE_OR_JOIN_MODE);
  activateOnFlag(ui->actionPaintbrush, m_Model, UIF_NOT_SNAKE_OR_JOIN_MODE);
  activateOnFlag(ui->actionJoin, m_Model, UIF_NOT_SNAKE_OR_JOIN_MODE);

  activateOnFlag(ui->action3DCrosshair, m_Model, UIF_BASEIMG_LOADED);
  activateOnFlag(ui->action3DTrackball, m_Model, UIF_BASEIMG_LOADED);
  activateOnFlag(ui->action3DScalpel, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);
  activateOnFlag(ui->action3DSpray, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);

  activateOnFlag(ui->actionLayerInspector, m_Model, UIF_BASEIMG_LOADED);
  activateOnFlag(ui->actionImage_Contrast, m_Model, UIF_BASEIMG_LOADED);
  activateOnFlag(ui->actionAutoContrastGlobal, m_Model, UIF_BASEIMG_LOADED);
  activateOnFlag(ui->actionResetContrastGlobal, m_Model, UIF_BASEIMG_LOADED);
  activateOnFlag(ui->actionColor_Map_Editor, m_Model, UIF_BASEIMG_LOADED);
  activateOnFlag(ui->actionLabel_Editor, m_Model, UIF_BASEIMG_LOADED);
  activateOnFlag(ui->actionImage_Information, m_Model, UIF_BASEIMG_LOADED);
  activateOnFlag(ui->actionLabel_Editor, m_Model, UIF_BASEIMG_LOADED);

  activateOnFlag(ui->actionReorient_Image, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);

  // Hook up toolbar actions to the toolbar
  makeActionGroupCoupling(this->GetMainToolActionGroup(),
                          m_Model->GetGlobalState()->GetToolbarModeModel());

  makeActionGroupCoupling(this->Get3DToolActionGroup(),
                          m_Model->GetGlobalState()->GetToolbarMode3DModel());


}

void MainImageWindow::ShowFirstTime()
{
  // Before showing the window, select the right pages for the main view
  // and the sidebar, otherwise we get an annoying flashing of windows at
  // startup, as the pages are changed in response to an event
  this->UpdateMainLayout();

  // Also make sure the other elements look right before showing the window
  this->UpdateRecentMenu();
  this->UpdateRecentProjectsMenu();
  this->UpdateWindowTitle();

  // Show the window
  this->show();
  this->raise();
}

// Slot for model updates
void MainImageWindow::onModelUpdate(const EventBucket &b)
{
  if(b.HasEvent(MainImageDimensionsChangeEvent()))
    {
    // Delaying the relayout of the main window seems to reduce the amount of
    // flashing that occurs when loading images.
    // TODO: figure out if we can avoid flashing altogether
    QTimer::singleShot(200, this, SLOT(UpdateMainLayout()));
    }

  if(b.HasEvent(LayerChangeEvent()) || b.HasEvent(WrapperMetadataChangeEvent()))
    {
    // Update the window title
    this->UpdateWindowTitle();
    }

  if(b.HasEvent(ValueChangedEvent(), m_Model->GetHistoryModel("MainImage")))
    {
    this->UpdateRecentMenu();
    }

  if(b.HasEvent(ValueChangedEvent(), m_Model->GetHistoryModel("Project")))
    {
    this->UpdateRecentProjectsMenu();
    }

  if(b.HasEvent(ValueChangedEvent(), m_Model->GetGlobalState()->GetProjectFilenameModel()))
    {
    this->UpdateProjectMenuItems();
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
    // Go to the splash page
    ui->stackMain->setCurrentWidget(ui->pageSplash);
    m_DockLeft->setWidget(m_SplashPanel);

    // Choose the appropriate page depending on whether there are recent images
    // available
    if(m_Model->IsHistoryEmpty("MainImage"))
      ui->tabSplash->setCurrentWidget(ui->tabGettingStarted);

    else if(ui->tabSplash->currentWidget() == ui->tabGettingStarted)
      ui->tabSplash->setCurrentWidget(ui->tabRecent);
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
  std::vector<std::string> recent = m_Model->GetRecentHistoryItems("MainImage", 5);

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

void MainImageWindow::UpdateRecentProjectsMenu()
{
  // Menus to populate
  QAction *menus[] = {
    ui->actionRecentWorkspace1,
    ui->actionRecentWorkspace2,
    ui->actionRecentWorkspace3,
    ui->actionRecentWorkspace4,
    ui->actionRecentWorkspace5};

  // List of filenames
  std::vector<std::string> recent = m_Model->GetRecentHistoryItems("Project", 5);

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
    mainfile = QFileInfo(from_utf8(gid->GetMain()->GetFileName())).fileName();
    segfile = QFileInfo(from_utf8(gid->GetSegmentation()->GetFileName())).fileName();
    }

  if(mainfile.length())
    {
    if(segfile.length())
      {
      this->setWindowTitle(QString("%1 - %2 - ITK-SNAP").arg(mainfile).arg(segfile));
      ui->actionSaveSegmentation->setText(QString("Save \"%1\"").arg(segfile));
      ui->actionSaveSegmentationAs->setText(QString("Save \"%1\" as...").arg(segfile));
      ui->actionSaveSegmentationAs->setVisible(true);
      }
    else
      {
      this->setWindowTitle(QString("%1 - New Segmentation - ITK-SNAP").arg(mainfile));
      ui->actionSaveSegmentation->setText(QString("Save Segmentation Image ..."));
      ui->actionSaveSegmentationAs->setVisible(false);
      }
    }
  else
    {
    this->setWindowTitle("ITK-SNAP");
    ui->actionSaveSegmentation->setText(QString("Save"));
    ui->actionSaveSegmentationAs->setText(QString("Save as..."));
    }
}

void MainImageWindow::UpdateProjectMenuItems()
{
  // Get the project filename
  QString project = from_utf8(m_Model->GetGlobalState()->GetProjectFilename());
  if(project.length())
    {
    // Get the filename without path
    ui->actionSaveWorkspace->setText(
          QString("Save Workspace \"%1\"").arg(QFileInfo(project).fileName()));
    }
  else
    {
    ui->actionSaveWorkspace->setText(QString("Save Workspace ..."));
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

void MainImageWindow::closeEvent(QCloseEvent *event)
{
  // Prompt for unsaved changes
  if(!SaveModifiedLayersDialog::PromptForUnsavedChanges(m_Model))
    {
    event->ignore();
    return;
    }

  // Close all the windows that are open
  QApplication::closeAllWindows();

  // Unload all images (this causes the associations to be saved)
  m_Model->GetDriver()->Quit();

  // Exit the application
  QCoreApplication::exit();
}

void MainImageWindow::on_actionQuit_triggered()
{ 
  // Close the main window
  this->close();
}

void MainImageWindow::on_actionLoad_from_Image_triggered()
{
  // Prompt for unsaved changes
  if(!SaveModifiedLayersDialog::PromptForUnsavedSegmentationChanges(m_Model))
    return;

  // Create a model for IO
  SmartPtr<LoadSegmentationImageDelegate> delegate = LoadSegmentationImageDelegate::New();
  delegate->Initialize(m_Model->GetDriver());
  SmartPtr<ImageIOWizardModel> model = ImageIOWizardModel::New();
  model->InitializeForLoad(m_Model, delegate,
                           "LabelImage", "Segmentation Image");

  // Execute the IO wizard
  ImageIOWizard wiz(this);
  wiz.SetModel(model);
  wiz.exec();
}


void MainImageWindow::on_actionImage_Contrast_triggered()
{
  // Go to the contrast page in the dialog
  m_LayerInspector->SetPageToContrastAdjustment();

  // Show the dialog
  RaiseDialog(m_LayerInspector);
}

void MainImageWindow::on_actionColor_Map_Editor_triggered()
{
  // Go to the contrast page in the dialog
  m_LayerInspector->SetPageToContrastAdjustment();

  // Show the dialog
  RaiseDialog(m_LayerInspector);
}

void MainImageWindow::on_actionImage_Information_triggered()
{
  // Go to the contrast page in the dialog
  m_LayerInspector->SetPageToImageInfo();

  // Show the dialog
  RaiseDialog(m_LayerInspector);
}

void MainImageWindow::on_actionLabel_Editor_triggered()
{
  // Execute the label editor
  RaiseDialog(m_LabelEditor);
}

void MainImageWindow::OpenSnakeWizard()
{
  // Initialize the snake wizard
  this->m_SnakeWizard->Initialize();

  // Remember the size of the window before the right dock was shown
  m_SizeWithoutRightDock = this->size();

  // Make the dock containing the wizard visible
  m_DockRight->setVisible(true);
}

void MainImageWindow::OpenGlobalWSWizard()
{
  // Initialize the GlobalWS wizard
  this->m_GlobalWSWizard->Initialize();

  // Remember the size of the window before the right dock was shown
  m_SizeWithoutRightDock = this->size();

  // Make the dock containing the wizard visible
  m_DockRight2->setVisible(true);
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
  // Check if the dropped file is a project
  if(m_Model->GetDriver()->IsProjectFile(to_utf8(file).c_str()))
    {
    // For the time being, the feature of opening the workspace in a new
    // window is not implemented. Instead, we just prompt the user for
    // unsaved changes.
    if(!SaveModifiedLayersDialog::PromptForUnsavedChanges(m_Model))
      return;

    // Load the project
    LoadProject(file);
    }

  else
    {
    if(m_Model->GetDriver()->IsMainImageLoaded())
      {
      // If an image is already loaded, we show the dialog
      m_DropDialog->SetDroppedFilename(file);
      m_DropDialog->setModal(true);
      RaiseDialog(m_DropDialog);
      }
    else
      {
      // Otherwise, attempt to load the image
      this->LoadMainImage(file);
      }
    }
}

void MainImageWindow::dropEvent(QDropEvent *event)
{
  QString file = event->mimeData()->urls().first().toLocalFile();
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

LayerInspectorDialog *MainImageWindow::GetLayerInspector()
{
  return m_LayerInspector;
}

void MainImageWindow::LoadMainImage(const QString &file)
{
  // Prompt for unsaved changes
  if(!SaveModifiedLayersDialog::PromptForUnsavedChanges(m_Model))
    return;

  // Try loading the image
  try
    {
    // Change cursor for this operation
    QtCursorOverride c(Qt::WaitCursor);
    IRISWarningList warnings;
    SmartPtr<LoadMainImageDelegate> del = LoadMainImageDelegate::New();
    del->Initialize(m_Model->GetDriver());
    m_Model->GetDriver()->LoadImageViaDelegate(file.toUtf8().constData(), del, warnings);
    }
  catch(exception &exc)
    {
    ReportNonLethalException(this, exc, "Image IO Error",
                             QString("Failed to load image %1").arg(file));
    }
}

void MainImageWindow::LoadRecentActionTriggered()
{
  // Get the filename that wants to be loaded
  QAction *action = qobject_cast<QAction *>(sender());
  QString file = action->text();
  LoadMainImage(file);
}

void MainImageWindow::LoadProject(const QString &file)
{
  // Try loading the image
  try
    {
    // Change cursor for this operation
    QtCursorOverride c(Qt::WaitCursor);
    IRISWarningList warnings;

    // Load the project
    m_Model->GetDriver()->OpenProject(to_utf8(file), warnings);
    }
  catch(exception &exc)
    {
    ReportNonLethalException(this, exc, "Error Opening Project",
                             QString("Failed to open project %1").arg(file));
  }
}

void MainImageWindow::onAnimationTimeout()
{
  if(m_Model)
    m_Model->AnimateLayerComponents();
}

void MainImageWindow::LoadRecentProjectActionTriggered()
{
  // Check for unsaved changes before loading new data
  if(!SaveModifiedLayersDialog::PromptForUnsavedChanges(m_Model))
    return;

  // Get the filename that wants to be loaded
  QAction *action = qobject_cast<QAction *>(sender());
  QString file = action->text();
  LoadProject(file);
}


void MainImageWindow::onSnakeWizardFinished()
{
  // Make the dock containing the wizard visible
  m_DockRight->setVisible(false);

  // TODO: this way of handling the size of the main window after the right
  // dock is hidden is rudimentary. I should learn how to use sizePolicy and
  // sizeHint fields more effectively.

  // Return to previous size
  this->layout()->activate();
  resize(m_SizeWithoutRightDock.width(), m_SizeWithoutRightDock.height());
}

void MainImageWindow::onGlobalWSWizardFinished()
{
  // Make the dock containing the wizard visible
  m_DockRight2->setVisible(false);

  // TODO: this way of handling the size of the main window after the right
  // dock is hidden is rudimentary. I should learn how to use sizePolicy and
  // sizeHint fields more effectively.

  // Return to previous size
  this->layout()->activate();
  resize(m_SizeWithoutRightDock.width(), m_SizeWithoutRightDock.height());
}

void MainImageWindow::on_actionUnload_All_triggered()
{
  // Prompt for unsaved changes
  if(!SaveModifiedLayersDialog::PromptForUnsavedChanges(m_Model))
    return;

  // Unload the main image
  m_Model->GetDriver()->UnloadMainImage();
}

void MainImageWindow::on_actionReorient_Image_triggered()
{
  // Show the reorientation dialog
  RaiseDialog(m_ReorientImageDialog);
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

void MainImageWindow::on_actionZoom_to_100_triggered()
{
  m_Model->GetSliceCoordinator()->SetZoomPercentageInAllWindows(1);
}

void MainImageWindow::on_actionZoom_to_200_triggered()
{
  m_Model->GetSliceCoordinator()->SetZoomPercentageInAllWindows(2);
}

void MainImageWindow::on_actionZoom_to_400_triggered()
{
  m_Model->GetSliceCoordinator()->SetZoomPercentageInAllWindows(4);
}


void MainImageWindow::on_actionUndo_triggered()
{
  m_Model->GetDriver()->Undo();
}

void MainImageWindow::on_actionRedo_triggered()
{
  m_Model->GetDriver()->Redo();
}

/** Filter application-level events
bool MainImageWindow::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::FileOpen)
    {
    QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(event);
    QMessageBox::information(this, "test", openEvent->url().path());
    if(openEvent->url().path() != "NULL")
      LoadDroppedFile(openEvent->url().path());
    return true;
    }
  else return false;
}
*/

void MainImageWindow::on_actionOpenMain_triggered()
{
  // Prompt for unsaved changes
  if(!SaveModifiedLayersDialog::PromptForUnsavedChanges(m_Model))
    return;

  // Create a model for IO
  SmartPtr<LoadMainImageDelegate> delegate = LoadMainImageDelegate::New();
  delegate->Initialize(m_Model->GetDriver());
  SmartPtr<ImageIOWizardModel> model = ImageIOWizardModel::New();
  model->InitializeForLoad(m_Model, delegate,
                           "AnatomicImage", "Main Image");

  // Execute the IO wizard
  ImageIOWizard wiz(this);
  wiz.SetModel(model);
  wiz.exec();

}

void MainImageWindow::on_actionAdd_Overlay_triggered()
{
  SmartPtr<LoadOverlayImageDelegate> delegate = LoadOverlayImageDelegate::New();
  delegate->Initialize(m_Model->GetDriver());
  SmartPtr<ImageIOWizardModel> model = ImageIOWizardModel::New();
  model->InitializeForLoad(m_Model, delegate,
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


#include "SynchronizationModel.h"

void MainImageWindow::ExportScreenshotSeries(AnatomicalDirection direction)
{
  // Get the corresponding window
  unsigned int iWindow =
      m_Model->GetDriver()->GetDisplayWindowForAnatomicalDirection(direction);

  // Get the corresponding image direction
  unsigned int iImageDir =
      m_Model->GetDriver()->GetImageDirectionForAnatomicalDirection(direction);

  // Browse for the output directory
  QString duser = QFileDialog::getExistingDirectory(
        this,
        "Directory where to save the screenshot series");

  if(!duser.length())
    return;

  // Generate the output filename
  const char *names[] = { "axial0001.png", "coronal0001.png", "sagittal0001.png" };
  std::string filename = to_utf8(QDir(duser).filePath(names[direction]));

  // back up cursor location
  Vector3ui xCrossImageOld = m_Model->GetDriver()->GetCursorPosition();
  Vector3ui xCrossImage = xCrossImageOld;
  Vector3ui xSize = m_Model->GetDriver()->GetCurrentImageData()->GetVolumeExtents();
  xCrossImage[iImageDir] = 0;

  // Get the panel that's saving
  SliceViewPanel *svp = reinterpret_cast<SliceViewPanel *>(m_ViewPanels[iWindow]);
  QtAbstractOpenGLBox *target = svp->GetSliceView();

  // turn sync off temporarily
  bool sync_state = m_Model->GetSynchronizationModel()->GetSyncEnabled();
  m_Model->GetSynchronizationModel()->SetSyncEnabled(false);

  for (size_t i = 0; i < xSize[iImageDir]; ++i)
  {
    // Set the cursor position
    m_Model->GetDriver()->SetCursorPosition(xCrossImage);

    // Repaint the GL window and save screenshot
    target->SaveScreenshot(filename);
    target->updateGL();
    // QCoreApplication::processEvents();

    // Go to the next slice
    xCrossImage[iImageDir]++;

    // Generate the next filename
    m_Model->SetLastScreenshotFileName(filename);
    filename = m_Model->GenerateScreenshotFilename();
  }

  // recover the original cursor position
  m_Model->GetDriver()->SetCursorPosition(xCrossImageOld);

  // turn sync back on
  m_Model->GetSynchronizationModel()->SetSyncEnabled(sync_state);
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

bool MainImageWindow::SaveSegmentation(bool interactive)
{
  return SaveImageLayer(
        m_Model, m_Model->GetDriver()->GetCurrentImageData()->GetSegmentation(),
        LABEL_ROLE, interactive, this);
}

void MainImageWindow::RaiseDialog(QDialog *dialog)
{
  dialog->show();
  dialog->activateWindow();
  dialog->raise();
}

void MainImageWindow::on_actionSaveSegmentation_triggered()
{
  SaveSegmentation(false);
}

void MainImageWindow::on_actionSaveSegmentationAs_triggered()
{
  SaveSegmentation(true);
}


void MainImageWindow::on_actionOverlayVisibilityToggleAll_triggered()
{
  m_Model->ToggleOverlayVisibility();
}

void MainImageWindow::on_actionOverlayVisibilityIncreaseAll_triggered()
{
  m_Model->AdjustOverlayOpacity(5);
}

void MainImageWindow::on_actionOverlayVisibilityDecreaseAll_triggered()
{
  m_Model->AdjustOverlayOpacity(-5);
}

void MainImageWindow::on_actionLayerInspector_triggered()
{
  // Show the dialog
  RaiseDialog(m_LayerInspector);
}

void MainImageWindow::on_actionAbout_triggered()
{
  // Show the about window
  RaiseDialog(m_AboutDialog);
}



void MainImageWindow::on_actionUnload_Last_Overlay_triggered()
{

}

void MainImageWindow::on_actionClear_triggered()
{
  // Prompt for unsaved changes
  if(!SaveModifiedLayersDialog::PromptForUnsavedSegmentationChanges(m_Model))
    return;

  m_Model->GetDriver()->ResetIRISSegmentationImage();
}

void MainImageWindow::on_actionSave_as_Mesh_triggered()
{
  MeshExportWizard wizard(this);
  wizard.SetModel(m_Model->GetMeshExportModel());
  wizard.exec();
}

void MainImageWindow::on_actionSaveMain_triggered()
{
  // This should only happen in SNAP mode
  assert(!m_Model->GetDriver()->IsSnakeModeActive());

  // Handle this through the layer manager
  ImageWrapperBase *wrapper =
      m_Model->GetDriver()->GetIRISImageData()->GetMain();
  QAction *save_action = m_LayerInspector->GetLayerSaveAction(wrapper);
  if(save_action)
    save_action->trigger();
}

#include "SNAPImageData.h"
void MainImageWindow::on_actionSaveSpeed_triggered()
{
  // This should only happen in SNAP mode
  assert(m_Model->GetDriver()->IsSnakeModeActive());

  // Handle this through the layer manager
  ImageWrapperBase *wrapper =
      m_Model->GetDriver()->GetSNAPImageData()->GetSpeed();
  QAction *save_action = m_LayerInspector->GetLayerSaveAction(wrapper);
  if(save_action)
    save_action->trigger();
}

void MainImageWindow::on_actionSaveLevelSet_triggered()
{
  // This should only happen in SNAP mode
  assert(m_Model->GetDriver()->IsSnakeModeLevelSetActive());

  // Handle this through the layer manager
  ImageWrapperBase *wrapper =
      m_Model->GetDriver()->GetSNAPImageData()->GetSnake();
  QAction *save_action = m_LayerInspector->GetLayerSaveAction(wrapper);
  if(save_action)
    save_action->trigger();
}

void MainImageWindow::on_actionSaveMainROI_triggered()
{
  // This should only happen in SNAP mode
  assert(m_Model->GetDriver()->IsSnakeModeActive());

  // Handle this through the layer manager
  ImageWrapperBase *wrapper =
      m_Model->GetDriver()->GetSNAPImageData()->GetMain();
  QAction *save_action = m_LayerInspector->GetLayerSaveAction(wrapper);
  if(save_action)
    save_action->trigger();
}

QSize MainImageWindow::sizeHint() const
{
  return QSize(900,700);
}

void MainImageWindow::on_actionPreferences_triggered()
{
  m_PreferencesDialog->ShowDialog();
}


void MainImageWindow::on_actionOpenWorkspace_triggered()
{
  // Check for unsaved changes before loading new data
  if(!SaveModifiedLayersDialog::PromptForUnsavedChanges(m_Model))
    return;

  // Use the dialog with history - to be consistent with other parts of SNAP
  QString file = ShowSimpleOpenDialogWithHistory(
        m_Model, "Project", "Open Workspace",
        "Workspace File", "ITK-SNAP Workspace Files (*.itksnap)");

  // If user hits cancel, move on
  if(file.isNull())
    return;

  // Make sure to get an absolute path, because the project needs that info
  QString file_abs = QFileInfo(file).absoluteFilePath();

  // Try loading the image
  try
    {
    // Change cursor for this operation
    QtCursorOverride c(Qt::WaitCursor);
    IRISWarningList warnings;

    // Load the project
    m_Model->GetDriver()->OpenProject(to_utf8(file_abs), warnings);
    }
  catch(exception &exc)
    {
    ReportNonLethalException(this, exc, "Error Opening Project",
                             QString("Failed to open project %1").arg(file_abs));
    }
}

bool MainImageWindow::SaveWorkspace(bool interactive)
{
  // Make sure that there are no unsaved changes. This is necessary before
  // a workspace can be saved. We disable the discard feature here because
  // the subsequent action does not close anything. The real purpose of this
  // dialog is to make sure each layer is assigned a name before saving the
  // workspace
  if(!SaveModifiedLayersDialog::PromptForUnsavedChanges(
       m_Model, NULL,
       SaveModifiedLayersDialog::DiscardDisabled
       | SaveModifiedLayersDialog::ProjectsDisabled))
    return false;

  // Use the global method
  return ::SaveWorkspace(m_Model, interactive, this);
}

void MainImageWindow::on_actionSaveWorkspace_triggered()
{
  SaveWorkspace(false);
}

void MainImageWindow::on_actionSaveWorkspaceAs_triggered()
{
  SaveWorkspace(true);
}


void MainImageWindow::ExportSlice(AnatomicalDirection direction)
{
  // Generate a default filename for this slice
  static const char *defpref[3] = {"axial", "sagittal", "coronal"};
  char deffn[40];

  // Figure out what slice it is
  size_t iSliceImg =
    m_Model->GetDriver()->GetImageDirectionForAnatomicalDirection(direction);

  sprintf(deffn,"%s_slice_%04d.png", defpref[direction],
    m_Model->GetDriver()->GetCursorPosition()[iSliceImg] + 1);

  // We need to get a filename for the export
  std::string fuser = to_utf8(
        QFileDialog::getSaveFileName(
          this, "Save Slice As", deffn,"Images (*.png *.jpg *.tiff)"));

  if(fuser.length())
    m_Model->GetDriver()->ExportSlice(direction, fuser.c_str());
}

void MainImageWindow::on_actionExportAxial_triggered()
{
  this->ExportSlice(ANATOMY_AXIAL);
}

void MainImageWindow::on_actionExportCoronal_triggered()
{
  this->ExportSlice(ANATOMY_CORONAL);
}

void MainImageWindow::on_actionExportSagittal_triggered()
{
  this->ExportSlice(ANATOMY_SAGITTAL);
}

void MainImageWindow::on_actionSSSeriesAxial_triggered()
{
  this->ExportScreenshotSeries(ANATOMY_AXIAL);
}

void MainImageWindow::on_actionSSSeriesCoronal_triggered()
{
  this->ExportScreenshotSeries(ANATOMY_CORONAL);
}

void MainImageWindow::on_actionSSSeriesSagittal_triggered()
{
  this->ExportScreenshotSeries(ANATOMY_SAGITTAL);
}

void MainImageWindow::on_actionForegroundLabelPrev_triggered()
{
  // Decrement the active label
  m_Model->IncrementDrawingColorLabel(-1);
}

void MainImageWindow::on_actionForegroundLabelNext_triggered()
{
  // Increment the active label
  m_Model->IncrementDrawingColorLabel(1);
}

void MainImageWindow::on_actionBackgroundLabelPrev_triggered()
{
  m_Model->IncrementDrawOverColorLabel(-1);
}

void MainImageWindow::on_actionBackgroundLabelNext_triggered()
{
  m_Model->IncrementDrawOverColorLabel(1);
}

void MainImageWindow::on_actionToggle_All_Annotations_triggered()
{
  // Toggle the overall visibility
  m_Model->GetAppearanceSettings()->SetOverallVisibility(
        !m_Model->GetAppearanceSettings()->GetOverallVisibility());
}

void MainImageWindow::on_actionToggle_Crosshair_triggered()
{
  // Toggle the crosshair visibility
  OpenGLAppearanceElement *elt = m_Model->GetAppearanceSettings()->GetUIElement(
        SNAPAppearanceSettings::CROSSHAIRS);
  elt->SetVisible(!elt->GetVisible());
}

void MainImageWindow::on_actionAnnotation_Preferences_triggered()
{
  // Show the preferences dialog
  m_PreferencesDialog->ShowDialog();
  m_PreferencesDialog->GoToAppearancePage();
}

void MainImageWindow::on_actionAutoContrastGlobal_triggered()
{
  // This triggers the autocontrast option for all layers.
  m_Model->AutoContrastAllLayers();
}

void MainImageWindow::on_actionResetContrastGlobal_triggered()
{
  // This triggers the autocontrast option for all layers.
  m_Model->ResetContrastAllLayers();
}

void MainImageWindow::DoUpdateCheck(bool quiet)
{
  std::string nver;

  // Check for the update
  SystemInterface::UpdateStatus  us =
      m_Model->GetSystemInterface()->CheckUpdate(nver, 1, 0, !quiet);

  // Communicate with the user
  if(us == SystemInterface::US_OUT_OF_DATE)
    {
    QMessageBox mbox(this);
    QPushButton *downloadButton = mbox.addButton("Open Download Page", QMessageBox::ActionRole);
    mbox.addButton("Not Now", QMessageBox::RejectRole);
    mbox.setIcon(QMessageBox::Question);
    mbox.setText(QString("A newer ITK-SNAP version (%1) is available.").arg(nver.c_str()));
    mbox.setInformativeText("Do you want to download the latest version?");
    mbox.setWindowTitle("ITK-SNAP Update Check");
    mbox.exec();

    if (mbox.clickedButton() == downloadButton)
      {
      QDesktopServices::openUrl(QUrl("http://www.itksnap.org/pmwiki/pmwiki.php?n=Downloads.SNAP3"));
      }
    }
  else if(us == SystemInterface::US_UP_TO_DATE && !quiet)
    {
    QMessageBox::information(this, "ITK-SNAP Update Check",
                             "Your version of ITK-SNAP is up to date!",
                             QMessageBox::Ok);
    }
  else if(us == SystemInterface::US_CONNECTION_FAILED && !quiet)
    {
    QMessageBox::warning(this,
                         "ITK-SNAP Update Check Failed",
                         "Could not connect to server. Go to itksnap.org to check if a new"
                         " version is available.");
    }
}

void MainImageWindow::UpdateAutoCheck()
{
  // Get the update state
  DefaultBehaviorSettings::UpdateCheckingPermission permission =
      m_Model->GetGlobalState()->GetDefaultBehaviorSettings()->GetCheckForUpdates();

  // If permission is unknown, prompt and change the setting
  if(permission == DefaultBehaviorSettings::UPDATE_UNKNOWN)
    {
    if(QMessageBox::Yes == QMessageBox::question(
         this, "Allow Automatic Update Checks?",
         "ITK-SNAP can check for software updates automatically.\n"
         "Do you want to enable this feature?",
         QMessageBox::Yes, QMessageBox::No))
      {
      permission = DefaultBehaviorSettings::UPDATE_YES;
      }
    else
      {
      permission = DefaultBehaviorSettings::UPDATE_NO;
      }
    m_Model->GetGlobalState()->GetDefaultBehaviorSettings()->SetCheckForUpdates(permission);
    }

  // Execute the update check
  if(permission == DefaultBehaviorSettings::UPDATE_YES)
    {
    DoUpdateCheck(true);
    }
}

void MainImageWindow::on_actionCheck_for_Updates_triggered()
{
  DoUpdateCheck(false);
}

void MainImageWindow::on_actionDocumentation_Home_triggered()
{
  QDesktopServices::openUrl(QUrl("http://www.itksnap.org/pmwiki/pmwiki.php?n=Documentation.SNAP3"));
}

void MainImageWindow::on_actionNew_ITK_SNAP_Window_triggered()
{
  // Launch a new SNAP
  std::list<std::string> args;
  m_Model->GetSystemInterface()->LaunchChildSNAPSimple(args);
}

void MainImageWindow::on_actionToggleJsrcVis_triggered()
{
    m_Model->ToggleJsrcVisibility();
}
