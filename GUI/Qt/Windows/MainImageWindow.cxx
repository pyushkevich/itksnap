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
#include "SynchronizationModel.h"

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
#include <InterpolateLabelsDialog.h>
#include "RegistrationDialog.h"

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
#include <QMimeData>
#include <QDesktopWidget>
#include <QShortcut>

#include <QTextStream>

QString read_tooltip_qt(const QString &filename)
{
  QFile file(filename);
  file.open(QFile::ReadOnly);
  QTextStream ts(&file);
  QString result = ts.readAll();
  file.close();;

  return result;
}

class ModeTooltipBuilder
{
public:
  ModeTooltipBuilder(QString title, QString descr)
  {
    m_Title = title;
    m_Descr = descr;

    // Read the HTML templates
    if(m_RowTemplate.isEmpty())
      {
      m_RowTemplate = ReadResource(":/html/TipTableRow");
      m_MainTemplate = ReadResource(":/html/ModeTipTemplate");
      }
  }

  enum Action { LMB, RMB, SCROLL };

  void addMouseAction(Action action, QString descr, QString modifier = "")
  {
    QString row = m_RowTemplate;
    QString atext;
    switch(action)
      {
      case ModeTooltipBuilder::LMB:
        atext = "left_click"; break;
      case ModeTooltipBuilder::RMB:
        atext = "right_click"; break;
      case ModeTooltipBuilder::SCROLL:
        atext = "scrolling"; break;
      }

    QString fullrow = row.arg(atext).arg(descr).arg(modifier);
    m_Rows.append(fullrow);
  }

  QString makeTooltip()
  {
    QString tooltip = m_MainTemplate;
    return tooltip.arg(m_Title, m_Descr, m_Rows);
  }

private:

  QString ReadResource(QString tag)
  {
    QFile file(tag);
    file.open(QFile::ReadOnly);
    QTextStream ts(&file);
    QString result = ts.readAll();
    file.close();
    return result;
  }


  QString m_Title, m_Descr, m_Rows;
  static QString m_RowTemplate, m_MainTemplate;
};

QString ModeTooltipBuilder::m_MainTemplate;
QString ModeTooltipBuilder::m_RowTemplate;


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
  ui->actionAnnotation->setActionGroup(grpToolbarMain);

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

  m_InterpolateLabelsDialog = new InterpolateLabelsDialog(this);
  m_InterpolateLabelsDialog->setModal(false);


  // Initialize the docked panels
  m_DockLeft = new QDockWidget(this);
  m_DockLeft->setAllowedAreas(Qt::LeftDockWidgetArea);
  m_DockLeft->setFeatures(
        QDockWidget::DockWidgetFloatable |
        QDockWidget::DockWidgetMovable);
  m_DockLeft->setWindowTitle("ITK-SNAP Toolbox");
  // m_DockLeft->setTitleBarWidget(new QWidget());
  this->addDockWidget(Qt::LeftDockWidgetArea, m_DockLeft);

  m_ControlPanel = new MainControlPanel(this);
  m_DockLeft->setWidget(m_ControlPanel);

  // Connect left dock to its menu item
  connect(m_DockLeft, SIGNAL(visibilityChanged(bool)),
          ui->actionMainControlPanel, SLOT(setChecked(bool)));

  // Set up the right hand side dock widget
  m_DockRight = new QDockWidget("Segment 3D", this);

  m_DockRight->setAllowedAreas(Qt::RightDockWidgetArea);
  m_DockRight->setFeatures(
        QDockWidget::DockWidgetFloatable |
        QDockWidget::DockWidgetMovable);

  m_RightDockStack = new QStackedWidget(m_DockRight);
  connect(m_RightDockStack, SIGNAL(currentChanged(int)),
          this, SLOT(onRightDockCurrentChanged(int)));

  m_DockRight->setWidget(m_RightDockStack);

  m_SnakeWizard = new SnakeWizardPanel(this);
  m_RegistrationDialog = new RegistrationDialog(this);

  m_RightDockStack->addWidget(m_SnakeWizard);
  m_RightDockStack->addWidget(m_RegistrationDialog);

  this->addDockWidget(Qt::RightDockWidgetArea, m_DockRight);

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

  // Hide the dock when the wizard finishes
  connect(m_SnakeWizard, SIGNAL(wizardFinished()),
          this, SLOT(onRightDockDialogFinished()));

  connect(m_RegistrationDialog, SIGNAL(wizardFinished()),
          this, SLOT(onRightDockDialogFinished()));

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

  // Set up the animation timer
  m_AnimateTimer = new QTimer(this);
  m_AnimateTimer->setInterval(1000);
  connect(m_AnimateTimer, SIGNAL(timeout()), SLOT(onAnimationTimeout()));

  // Start the timer (it doesn't cost much...)
  m_AnimateTimer->start();

  // Create keyboard shortcuts for opacity (because there seems to be a bug/feature on MacOS
  // where keyboard shortcuts require the Fn-key to be pressed in QMenu
  this->HookupShortcutToAction(QKeySequence("s"), ui->actionSegmentationToggle);
  this->HookupShortcutToAction(QKeySequence("a"), ui->actionSegmentationDecreaseOpacity);
  this->HookupShortcutToAction(QKeySequence("d"), ui->actionSegmentationIncreaseOpacity);
  this->HookupShortcutToAction(QKeySequence("q"), ui->actionOverlayVisibilityDecreaseAll);
  this->HookupShortcutToAction(QKeySequence("w"), ui->actionOverlayVisibilityToggleAll);
  this->HookupShortcutToAction(QKeySequence("e"), ui->actionOverlayVisibilityIncreaseAll);
  this->HookupShortcutToAction(QKeySequence(Qt::Key_X), ui->actionToggle_All_Annotations);
  this->HookupShortcutToAction(QKeySequence(Qt::SHIFT + Qt::Key_X), ui->actionToggle_Crosshair);
  this->HookupShortcutToAction(QKeySequence("<"), ui->actionForegroundLabelPrev);
  this->HookupShortcutToAction(QKeySequence(">"), ui->actionForegroundLabelNext);
  this->HookupSecondaryShortcutToAction(QKeySequence(","), ui->actionForegroundLabelPrev);
  this->HookupSecondaryShortcutToAction(QKeySequence("."), ui->actionForegroundLabelNext);
  this->HookupShortcutToAction(QKeySequence("C"), ui->actionCenter_on_Cursor);

  this->HookupShortcutToAction(QKeySequence("\\"), ui->actionToggleLayerLayout);
  this->HookupShortcutToAction(QKeySequence("["), ui->actionActivatePreviousLayer);
  this->HookupShortcutToAction(QKeySequence("]"), ui->actionActivateNextLayer);

  // Common modifiers
  const QString mod_option(QChar(0x2325));
  const QString mod_shift(QChar(0x21e7));

  // Generate tooltips for the complex actions
  ModeTooltipBuilder ttCrosshair("Crosshair Mode (1)",
                                 "Used to position the 3D cursor in the three orthogonal image slices.");
  ttCrosshair.addMouseAction(ModeTooltipBuilder::LMB, "<b>Place and move the 3D cursor</b>");
  ttCrosshair.addMouseAction(ModeTooltipBuilder::RMB, "Zoom in and out (hold & drag)");
  ttCrosshair.addMouseAction(ModeTooltipBuilder::LMB, "Pan (hold & drag)",mod_option);
  ttCrosshair.addMouseAction(ModeTooltipBuilder::SCROLL, "Go to next/previous image slice");
  ttCrosshair.addMouseAction(ModeTooltipBuilder::SCROLL, "Go to next/previous image component",mod_shift);
  ui->actionCrosshair->setToolTip(ttCrosshair.makeTooltip());

  ModeTooltipBuilder ttZoom("Zoom/Pan Mode (2)",
                            "Used to zoom into the image and to pan around when zoomed in.");
  ttZoom.addMouseAction(ModeTooltipBuilder::LMB, "<b>Pan (hold & drag)</b>");
  ttZoom.addMouseAction(ModeTooltipBuilder::RMB, "<b>Zoom in and out (hold & drag)</b>");
  ttZoom.addMouseAction(ModeTooltipBuilder::LMB, "Place and move the 3D cursor", mod_option);
  ttZoom.addMouseAction(ModeTooltipBuilder::SCROLL, "Scroll through image slices");
  ttZoom.addMouseAction(ModeTooltipBuilder::SCROLL, "Scroll through image components",mod_shift);
  ui->actionZoomPan->setToolTip(ttZoom.makeTooltip());

  ModeTooltipBuilder ttPolygon("Polygon Mode (3)",
                               "Used to perform manual segmentation by drawing and filling polygons in the three orthogonal image slices.");
  ttPolygon.addMouseAction(ModeTooltipBuilder::LMB, "<b>Add points to the polygon and edit the completed polygon</b>");
  ttPolygon.addMouseAction(ModeTooltipBuilder::RMB, "Zoom in and out (hold & drag)");
  ttPolygon.addMouseAction(ModeTooltipBuilder::LMB, "Place and move the 3D cursor",mod_option);
  ttPolygon.addMouseAction(ModeTooltipBuilder::SCROLL, "Scroll through image slices");
  ttPolygon.addMouseAction(ModeTooltipBuilder::SCROLL, "Scroll through image components",mod_shift);
  ui->actionPolygon->setToolTip(ttPolygon.makeTooltip());

  ModeTooltipBuilder ttPaintbrush("Paintbrush Mode (4)",
                               "Used to perform manual segmentation by drawing with a paintbrush-like tool. "
                               "Different brush shapes are available, including an adaptive brush that adjusts itself to the image data.");
  ttPaintbrush.addMouseAction(ModeTooltipBuilder::LMB, "<b>Paint with the active label</b>");
  ttPaintbrush.addMouseAction(ModeTooltipBuilder::RMB, "<b>Erase voxels painted with the active label</b>");
  ttPaintbrush.addMouseAction(ModeTooltipBuilder::LMB, "Place and move the 3D cursor",mod_option);
  ttPaintbrush.addMouseAction(ModeTooltipBuilder::SCROLL, "Scroll through image slices");
  ttPaintbrush.addMouseAction(ModeTooltipBuilder::SCROLL, "Scroll through image components",mod_shift);
  ui->actionPaintbrush->setToolTip(ttPaintbrush.makeTooltip());

  ModeTooltipBuilder ttSnake("Active Contour (aka \"Snake\") Segmentation Mode (5)",
                             "Used to select the region of interest for semi-automatic active contour "
                             "segmentation and start the semi-automatic segmentation wizard.");
  ttSnake.addMouseAction(ModeTooltipBuilder::LMB, "<b>Adjust the boundaries of the region of interest</b>");
  ttSnake.addMouseAction(ModeTooltipBuilder::RMB, "Zoom in and out (hold & drag)");
  ttSnake.addMouseAction(ModeTooltipBuilder::LMB, "Place and move the 3D cursor",mod_option);
  ttSnake.addMouseAction(ModeTooltipBuilder::SCROLL, "Scroll through image slices");
  ttSnake.addMouseAction(ModeTooltipBuilder::SCROLL, "Scroll through image components",mod_shift);
  ui->actionSnake->setToolTip(ttSnake.makeTooltip());

  ModeTooltipBuilder ttRuler("Image Annotation Mode (6)",
                             "Used to draw annotations (lines, text) on image slices and to measure "
                             "distances and angles between points in a slice.");
  ttRuler.addMouseAction(ModeTooltipBuilder::LMB, "<b>Draw and edit annotations</b>");
  ttRuler.addMouseAction(ModeTooltipBuilder::RMB, "Zoom in and out (hold & drag)");
  ttRuler.addMouseAction(ModeTooltipBuilder::LMB, "Place and move the 3D cursor",mod_option);
  ttRuler.addMouseAction(ModeTooltipBuilder::SCROLL, "Scroll through image slices");
  ttRuler.addMouseAction(ModeTooltipBuilder::SCROLL, "Scroll through image components",mod_shift);
  ui->actionAnnotation->setToolTip(ttRuler.makeTooltip());

  // Translate the tooltips in all the widgets. This changes the apple symbols that are currently
  // hard coded in the tooltips into their Windows/Linux equivalents
#ifndef __APPLE__
  TranslateChildTooltipKeyModifiers(this);
#endif

  // Listen to changes in the active window. This affects the behavior of the "close" shortcut
  connect(qApp, SIGNAL(focusChanged(QWidget*,QWidget*)), this, SLOT(onActiveChanged()));

  // Start with the "close window" menu item hidden
  ui->actionClose_Window->setVisible(false);
}


MainImageWindow::~MainImageWindow()
{
  delete m_ProgressReporterDelegate;
  delete ui;
}

void MainImageWindow::HookupShortcutToAction(const QKeySequence &ks, QAction *action)
{
  // The bug/feature of single-key shortcuts not working is only in MacOS
#if QT_VERSION >= 0x050000 && QT_VERSION < 0x050600 && defined __APPLE__
  QShortcut *short_S = new QShortcut(ks, this);
  connect(short_S, SIGNAL(activated()), action, SLOT(trigger()));
#endif
}

void MainImageWindow::HookupSecondaryShortcutToAction(const QKeySequence &ks, QAction *action)
{
  QShortcut *short_S = new QShortcut(ks, this);
  connect(short_S, SIGNAL(activated()), action, SLOT(trigger()));
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
  m_PreferencesDialog->SetModel(model->GetGlobalPreferencesModel());
  m_InterpolateLabelsDialog->SetModel(model->GetInterpolateLabelModel());
  m_RegistrationDialog->SetModel(model->GetRegistrationModel());

  // Initialize the docked panels
  m_ControlPanel->SetModel(model);

  // Attach the progress reporter delegate to the model
  m_Model->SetProgressReporterDelegate(m_ProgressReporterDelegate);

  // Listen for changes to the main image, updating the recent image file
  // menu. TODO: a more direct way would be to listen to changes to the
  // history, but that requires making history an event-firing object
  LatentITKEventNotifier::connect(model->GetDriver(), LayerChangeEvent(),
                                  this, SLOT(onModelUpdate(EventBucket)));

  // Also listen to changes in the image filenames
  LatentITKEventNotifier::connect(model->GetDriver(), WrapperMetadataChangeEvent(),
                                  this, SLOT(onModelUpdate(EventBucket)));

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

  // Listen to changes in the display layout to adjust the dimensions of the main
  // window in response.
  LatentITKEventNotifier::connect(
        model->GetDisplayLayoutModel(), DisplayLayoutModel::DisplayLayoutChangeEvent(),
        this, SLOT(onModelUpdate(EventBucket)));

  LatentITKEventNotifier::connect(
        model->GetDisplayLayoutModel(), DisplayLayoutModel::LayerLayoutChangeEvent(),
        this, SLOT(onModelUpdate(EventBucket)));

  // Watch for changes in the selected layer
  LatentITKEventNotifier::connect(
        model->GetDriver()->GetGlobalState()->GetSelectedLayerIdModel(),
        ValueChangedEvent(), this, SLOT(onModelUpdate(EventBucket)));

  // Couple the visibility of each view panel to the correponding property
  // model in DisplayLayoutModel
  DisplayLayoutModel *layoutModel = m_Model->GetDisplayLayoutModel();
  for(int i = 0; i < 4; i++)
    {
    makeWidgetVisibilityCoupling(m_ViewPanels[i],
                                 layoutModel->GetViewPanelVisibilityModel(i));
    }

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
  activateOnFlag(ui->actionForegroundLabelNext, m_Model, UIF_BASEIMG_LOADED);
  activateOnFlag(ui->actionForegroundLabelPrev, m_Model, UIF_BASEIMG_LOADED);
  activateOnFlag(ui->actionBackgroundLabelNext, m_Model, UIF_BASEIMG_LOADED);
  activateOnFlag(ui->actionBackgroundLabelPrev, m_Model, UIF_BASEIMG_LOADED);

  activateOnFlag(ui->actionToggleLayerLayout, m_Model, UIF_MULTIPLE_BASE_LAYERS);
  activateOnFlag(ui->actionActivateNextLayer, m_Model, UIF_MULTIPLE_BASE_LAYERS);
  activateOnFlag(ui->actionActivatePreviousLayer, m_Model, UIF_MULTIPLE_BASE_LAYERS);

  // Add actions that are not on the menu
  activateOnFlag(ui->actionZoomToFitInAllViews, m_Model, UIF_BASEIMG_LOADED);
  activateOnFlag(ui->actionCenter_on_Cursor, m_Model, UIF_BASEIMG_LOADED);
  activateOnFlag(ui->actionZoom_to_100, m_Model, UIF_BASEIMG_LOADED);
  activateOnFlag(ui->actionZoom_to_200, m_Model, UIF_BASEIMG_LOADED);
  activateOnFlag(ui->actionZoom_to_400, m_Model, UIF_BASEIMG_LOADED);


  // Set up activations - Segmentation menu
  activateOnFlag(ui->actionLoad_from_Image, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);
  activateOnFlag(ui->actionClear, m_Model, UIF_BASEIMG_LOADED);
  activateOnFlag(ui->actionSaveSegmentation, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);
  activateOnFlag(ui->actionSaveSegmentationAs, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);
  activateOnFlag(ui->actionSave_as_Mesh, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);
  activateOnFlag(ui->actionLoadLabels, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);
  activateOnFlag(ui->actionSaveLabels, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);
  activateOnFlag(ui->actionVolumesAndStatistics, m_Model, UIF_BASEIMG_LOADED);
  activateOnFlag(ui->menuAppearance, m_Model, UIF_BASEIMG_LOADED);

  // Overlay action activations
  activateOnFlag(ui->actionAdd_Overlay, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);
  // activateOnAllFlags(ui->actionUnload_Last_Overlay, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED, UIF_OVERLAY_LOADED);
  activateOnAllFlags(ui->actionUnload_All_Overlays, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED, UIF_OVERLAY_LOADED);
  activateOnFlag(ui->actionOverlayVisibilityToggleAll, m_Model, UIF_OVERLAY_LOADED);
  activateOnFlag(ui->actionOverlayVisibilityDecreaseAll, m_Model, UIF_OVERLAY_LOADED);
  activateOnFlag(ui->actionOverlayVisibilityIncreaseAll, m_Model, UIF_OVERLAY_LOADED);

  // Workspace menu
  activateOnFlag(ui->actionOpenWorkspace, m_Model, UIF_IRIS_MODE);
  activateOnFlag(ui->actionSaveWorkspace, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);
  activateOnFlag(ui->actionSaveWorkspaceAs, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);

  // Tool action activations
  activateOnFlag(ui->actionCrosshair, m_Model, UIF_BASEIMG_LOADED);
  activateOnFlag(ui->actionZoomPan, m_Model, UIF_BASEIMG_LOADED);
  activateOnFlag(ui->actionPolygon, m_Model, UIF_BASEIMG_LOADED);
  activateOnFlag(ui->actionSnake, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);
  activateOnFlag(ui->actionPaintbrush, m_Model, UIF_BASEIMG_LOADED);
  activateOnFlag(ui->actionAnnotation, m_Model, UIF_BASEIMG_LOADED);

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
  activateOnFlag(ui->actionRegistration, m_Model, UIF_IRIS_WITH_OVERLAY_LOADED);

  activateOnFlag(ui->actionReorient_Image, m_Model, UIF_IRIS_WITH_BASEIMG_LOADED);

  // Hook up toolbar actions to the toolbar
  makeActionGroupCoupling(this->GetMainToolActionGroup(),
                          m_Model->GetGlobalState()->GetToolbarModeModel());

  makeActionGroupCoupling(this->Get3DToolActionGroup(),
                          m_Model->GetGlobalState()->GetToolbarMode3DModel());

  // Set the synchronization state
  m_Model->GetSynchronizationModel()->SetCanBroadcast(this->isActiveWindow());
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
  this->UpdateLayerLayoutActions();
  this->UpdateSelectedLayerActions();
  this->UpdateDICOMContentsMenu();

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
    // QTimer::singleShot(200, this, SLOT(UpdateMainLayout()));
    this->UpdateMainLayout();
    }

  if(b.HasEvent(LayerChangeEvent()) || b.HasEvent(WrapperMetadataChangeEvent()))
    {
    // Update the window title
    this->UpdateWindowTitle();
    this->UpdateSelectedLayerActions();
    }

  if(b.HasEvent(LayerChangeEvent()) ||
     b.HasEvent(ValueChangedEvent(), m_Model->GetHistoryModel("MainImage")))
    {
    this->UpdateRecentMenu();
    this->UpdateDICOMContentsMenu();
    }

  if(b.HasEvent(ValueChangedEvent(), m_Model->GetHistoryModel("Project")))
    {
    this->UpdateRecentProjectsMenu();
    }

  if(b.HasEvent(ValueChangedEvent(), m_Model->GetGlobalState()->GetProjectFilenameModel()))
    {
    this->UpdateWindowTitle();
    this->UpdateProjectMenuItems();
    }

  if(b.HasEvent(DisplayLayoutModel::DisplayLayoutChangeEvent()))
    {
    this->UpdateCanvasDimensions();
    }

  if(b.HasEvent(DisplayLayoutModel::LayerLayoutChangeEvent()))
    {
    this->UpdateLayerLayoutActions();
    }

  if(b.HasEvent(ValueChangedEvent(),
                m_Model->GetDriver()->GetGlobalState()->GetSelectedLayerIdModel()))
    {
    this->UpdateSelectedLayerActions();
    }
}

void MainImageWindow::externalStyleSheetFileChanged(const QString &file)
{
  QFile File(file);
  File.open(QFile::ReadOnly);
  this->setStyleSheet(QLatin1String(File.readAll()));
}

void MainImageWindow::onActiveChanged()
{
  if(this->isActiveWindow())
    {
    ui->actionUnload_Last_Overlay->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_W));
    ui->actionClose_Window->setVisible(false);
    }
  else
    {
    ui->actionUnload_Last_Overlay->setShortcut(QKeySequence());
    ui->actionClose_Window->setVisible(true);
    }
}

void MainImageWindow::UpdateMainLayout()
{
  // Update the image dimensions
  this->UpdateCanvasDimensions();

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


void MainImageWindow::UpdateCanvasDimensions()
{
  // We should not do this in fullscreen mode
  Qt::WindowStates ws = this->windowState();
  if(ws.testFlag(Qt::WindowFullScreen))
    {
    return;
    }

  // Get the current desktop dimensions
  QRect desktop = QApplication::desktop()->availableGeometry(this);

  // The desired window aspect ratio
  double windowAR = 1.0;

  // Get the current aspect ratio
  if(m_Model->GetDriver()->IsMainImageLoaded())
    {
    if(m_Model->GetDisplayLayoutModel()->GetSliceViewLayerLayoutModel()->GetValue() == LAYOUT_TILED)
      {
      Vector2ui tiling =
          m_Model->GetDisplayLayoutModel()->GetSliceViewLayerTilingModel()->GetValue();

      // Compute the tiling aspect ratio
      double tilingAR = tiling(1) * 1.0 / tiling(0);

      // The tiling aspect ratio should not be mapped directly to the screen aspect ratio -
      // this creates configurations that are too wide. Instead, we will use a scaling factor
      windowAR = (tilingAR - 1.0) * 0.6 + 1.0;
      }
    else if(m_Model->GetDisplayLayoutModel()->GetNumberOfGroundLevelLayers() > 1)
      {
      windowAR = 1.0 / 0.88;
      }
    }

  // Adjust the width of the screen to achieve desired aspect ratio
  int cw_width = static_cast<int>(windowAR * ui->centralwidget->height());
  int mw_width = this->width() + (cw_width - ui->centralwidget->width());

  // Adjust the width to be within the desktop dimensions
  mw_width = std::min(desktop.width(), mw_width);

  // Deterimine the left point
  int left = std::max(0, this->pos().x() + this->width() / 2 - mw_width / 2);

  // Adjust the left point if necessary
  if(left + mw_width > desktop.right())
    left = std::max(0, desktop.right() - mw_width);

  // Now we want to position the window nicely.
  this->resize(QSize(mw_width, this->height()));
  this->move(left, this->pos().y());
}


void MainImageWindow::UpdateLayerLayoutActions()
{
  DisplayLayoutModel *dlm = m_Model->GetDisplayLayoutModel();
  LayerLayout ll = dlm->GetSliceViewLayerLayoutModel()->GetValue();
  if(ll == LAYOUT_TILED)
    {
    ui->actionToggleLayerLayout->setIcon(QIcon(":/root/layout_thumb_16.png"));
    ui->actionToggleLayerLayout->setText("Enter Thumbnail Layout");
    }
  else if(ll == LAYOUT_STACKED)
    {
    ui->actionToggleLayerLayout->setIcon(QIcon(":/root/layout_tile_16.png"));
    ui->actionToggleLayerLayout->setText("Enter Tiled Layout");
    }
}

void MainImageWindow::UpdateSelectedLayerActions()
{
  // Find the selected layer
  ImageWrapperBase *layer =
      m_Model->GetDriver()->GetCurrentImageData()->FindLayer(
        m_Model->GetGlobalState()->GetSelectedLayerId(), false);

  if(layer)
    {
    ui->actionUnload_Last_Overlay->setVisible(true);
    ui->actionUnload_Last_Overlay->setEnabled(true);
    ui->actionUnload_Last_Overlay->setText(
          QString("Close image \"%1\"").arg(from_utf8(layer->GetNickname())));
    }
  else
    {
    ui->actionUnload_Last_Overlay->setVisible(false);
    ui->actionUnload_Last_Overlay->setEnabled(false);
    ui->actionUnload_Last_Overlay->setText("Close selected image");
    }
}

Q_DECLARE_METATYPE(IRISApplication::DicomSeriesDescriptor)

void MainImageWindow::UpdateDICOMContentsMenu()
{
  // Clear the menu
  ui->menuAddAnotherDicomImage->clear();

  // Any actions added?
  bool have_actions = false;

  // Get the list of dicom series grouped by filename
  IRISApplication::DicomSeriesTree dicoms =
      m_Model->GetDriver()->ListAvailableSiblingDicomSeries();

  // Iterate over all of these
  for(IRISApplication::DicomSeriesTree::const_iterator it_map = dicoms.begin();
      it_map != dicoms.end(); ++it_map)
    {
    // Create a submenu or point to the menu itself
    QMenu *target_menu = ui->menuAddAnotherDicomImage;
    if(dicoms.size() > 1)
      {
      target_menu = new QMenu(from_utf8(it_map->first), ui->menuAddAnotherDicomImage);
      ui->menuAddAnotherDicomImage->addMenu(target_menu);
      }

    // Add all the series_ids as actions
    for(IRISApplication::DicomSeriesListing::const_iterator it_list =
        it_map->second.begin(); it_list != it_map->second.end(); it_list++)
      {
      // Create a new action
      QAction *action = new QAction(this);
      QVariant user_data; user_data.setValue(*it_list);
      action->setData(user_data);
      action->setText(QString("%1 [%2]")
                      .arg(from_utf8(it_list->series_desc))
                      .arg(from_utf8(it_list->dimensions)));

      // Connect this action to its slot
      connect(action, SIGNAL(triggered()),
              this, SLOT(LoadAnotherDicomActionTriggered()));

      // Add the action to the menu
      target_menu->addAction(action);

      // We have some actions!
      have_actions = true;
      }
    }

  // Hide or show the menu based on availability of actions
  ui->menuAddAnotherDicomImage->menuAction()->setVisible(have_actions);
}

void MainImageWindow::CreateRecentMenu(
    QMenu *submenu,
    const char *history_category,
    bool use_global_history,
    int max_items,
    const char *slot)
{
  // Delete all the menu items in the parent menu
  submenu->clear();

  // Get the recent history for this category
  std::vector<std::string> recent =
      m_Model->GetRecentHistoryItems(history_category, max_items, use_global_history);

  // Create an action for each recent item
  for(int i = 0; i < recent.size(); i++)
    {
    // Create an action for this file
    QAction *action = submenu->addAction(from_utf8(recent[i]));
    connect(action, SIGNAL(triggered(bool)), this, slot);
    }

  // Toggle the visibility of the submenu
  submenu->menuAction()->setVisible(recent.size() > 0);
}

void MainImageWindow::UpdateRecentMenu()
{
  // Create recent menus for various history categories
  this->CreateRecentMenu(ui->menuRecent_Images, "MainImage", true, 5,
                         SLOT(LoadRecentActionTriggered()));

  this->CreateRecentMenu(ui->menuRecent_Overlays, "AnatomicImage", false, 5,
                         SLOT(LoadRecentOverlayActionTriggered()));

  this->CreateRecentMenu(ui->menuRecent_Segmentations, "LabelImage", false, 5,
                         SLOT(LoadRecentSegmentationActionTriggered()));
}

void MainImageWindow::UpdateRecentProjectsMenu()
{
  this->CreateRecentMenu(ui->menuRecentWorkspaces, "Project", true, 5,
                         SLOT(LoadRecentProjectActionTriggered()));
}



void MainImageWindow::UpdateWindowTitle()
{
  GenericImageData *gid = m_Model->GetDriver()->GetIRISImageData();
  QString mainfile, segfile, projfile;
  if(gid && gid->IsMainLoaded())
    {
    mainfile = QFileInfo(from_utf8(gid->GetMain()->GetFileName())).fileName();
    segfile = QFileInfo(from_utf8(gid->GetSegmentation()->GetFileName())).fileName();
    }

  // If a project is loaded, we display the project title
  if(m_Model->GetGlobalState()->GetProjectFilename().length())
    projfile = QFileInfo(from_utf8(m_Model->GetGlobalState()->GetProjectFilename())).fileName();

  // Set up the window title
  if(projfile.length())
    {
    this->setWindowTitle(QString("%1 - ITK-SNAP").arg(projfile));
    }
  else if(mainfile.length() && segfile.length())
    {
    this->setWindowTitle(QString("%1 - %2 - ITK-SNAP").arg(mainfile).arg(segfile));
    }
  else if(mainfile.length())
    {
    this->setWindowTitle(QString("%1 - New Segmentation - ITK-SNAP").arg(mainfile));
    }
  else
    {
    this->setWindowTitle("ITK-SNAP");
    }

  // Set up the save segmentation menu items
  if(segfile.length())
    {
    ui->actionSaveSegmentation->setText(QString("Save \"%1\"").arg(segfile));
    ui->actionSaveSegmentationAs->setText(QString("Save \"%1\" as...").arg(segfile));
    ui->actionSaveSegmentationAs->setVisible(true);
    }
  else if(mainfile.length())
    {
    ui->actionSaveSegmentation->setText(QString("Save Segmentation Image ..."));
    ui->actionSaveSegmentationAs->setVisible(false);
    }
  else
    {
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
  model->InitializeForLoad(m_Model, delegate);

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
  m_DockRight->setWindowTitle("Segment 3D");
  m_RightDockStack->setCurrentWidget(m_SnakeWizard);
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
      // Otherwise, load the main image directly
      m_DropDialog->LoadMainImage(file);
      }
    }
}

#ifdef __APPLE__
#include <CoreFoundation/CFError.h>
#include <CoreFoundation/CFURL.h>
#endif

void MainImageWindow::dropEvent(QDropEvent *event)
{
  QUrl url = event->mimeData()->urls().first();

#if defined(__APPLE__) && QT_VERSION >= 0x050000
  // TODO: this is a Yosemite bug fix - bug https://bugreports.qt.io/browse/QTBUG-40449
  // Check if this is still necessary in future Qt versions (discovered in Qt 5.4)
  if (url.toString().startsWith("file:///.file/id="))
    {
    CFURLRef cfurl = url.toCFURL();
    CFErrorRef error = 0;
    CFURLRef absurl = CFURLCreateFilePathURL(kCFAllocatorDefault, cfurl, &error);
    url = QUrl::fromCFURL(absurl);
    CFRelease(cfurl);
    CFRelease(absurl);
    }

#elif defined(__APPLE__) && QT_VERSION < 0x050000

  QString localFileQString = url.toLocalFile();
  // [pzion 20150805] Work around
  // https://bugreports.qt.io/browse/QTBUG-40449
  if ( localFileQString.startsWith("/.file/id=") )
    {
    CFStringRef relCFStringRef =
        CFStringCreateWithCString(
          kCFAllocatorDefault,
          localFileQString.toUtf8().constData(),
          kCFStringEncodingUTF8
          );
    CFURLRef relCFURL =
        CFURLCreateWithFileSystemPath(
          kCFAllocatorDefault,
          relCFStringRef,
          kCFURLPOSIXPathStyle,
          false // isDirectory
          );
    CFErrorRef error = 0;
    CFURLRef absCFURL =
        CFURLCreateFilePathURL(
          kCFAllocatorDefault,
          relCFURL,
          &error
          );
    if ( !error )
      {
      static const CFIndex maxAbsPathCStrBufLen = 4096;
      char absPathCStr[maxAbsPathCStrBufLen];
      if ( CFURLGetFileSystemRepresentation(
             absCFURL,
             true, // resolveAgainstBase
             reinterpret_cast<UInt8 *>( &absPathCStr[0] ),
             maxAbsPathCStrBufLen
             ) )
        {
        localFileQString = QString( absPathCStr );
        }
      }
    CFRelease( absCFURL );
    CFRelease( relCFURL );
    CFRelease( relCFStringRef );

    url = QUrl::fromLocalFile(localFileQString);
    }

#endif

  QString file = url.toLocalFile();
  event->acceptProposedAction();
  LoadDroppedFile(file);
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

void MainImageWindow::LoadRecentOverlayActionTriggered()
{
  // Get the filename that wants to be loaded
  QAction *action = qobject_cast<QAction *>(sender());
  QString file = action->text();

  // Try loading the image
  try
    {
    // Change cursor for this operation
    QtCursorOverride c(Qt::WaitCursor);
    IRISWarningList warnings;
    SmartPtr<LoadOverlayImageDelegate> del = LoadOverlayImageDelegate::New();
    del->Initialize(m_Model->GetDriver());
    m_Model->GetDriver()->LoadImageViaDelegate(file.toUtf8().constData(), del, warnings);
    }
  catch(exception &exc)
    {
    ReportNonLethalException(this, exc, "Image IO Error",
                             QString("Failed to load overlay image %1").arg(file));
    }
}

void MainImageWindow::LoadRecentSegmentationActionTriggered()
{
  // Get the filename that wants to be loaded
  QAction *action = qobject_cast<QAction *>(sender());
  QString file = action->text();

  // Prompt for unsaved changes
  if(!SaveModifiedLayersDialog::PromptForUnsavedSegmentationChanges(m_Model))
    return;

  // Try loading the image
  try
    {
    // Change cursor for this operation
    QtCursorOverride c(Qt::WaitCursor);
    IRISWarningList warnings;
    SmartPtr<LoadSegmentationImageDelegate> del = LoadSegmentationImageDelegate::New();
    del->Initialize(m_Model->GetDriver());
    m_Model->GetDriver()->LoadImageViaDelegate(file.toUtf8().constData(), del, warnings);
    }
  catch(exception &exc)
    {
    ReportNonLethalException(this, exc, "Image IO Error",
                             QString("Failed to load segmentation image %1").arg(file));
    }
}

void MainImageWindow::LoadAnotherDicomActionTriggered()
{
  // Request to load another DICOM from the main image's folder
  QAction *action = qobject_cast<QAction *>(sender());

  // Get the dicom descriptor
  IRISApplication::DicomSeriesDescriptor desc =
      action->data().value<IRISApplication::DicomSeriesDescriptor>();

  // Try to load a DICOM with this series ID
  try
    {
    // Change cursor for this operation
    QtCursorOverride c(Qt::WaitCursor);
    IRISWarningList warnings;
    SmartPtr<LoadOverlayImageDelegate> del = LoadOverlayImageDelegate::New();
    del->Initialize(m_Model->GetDriver());
    m_Model->GetDriver()->LoadAnotherDicomSeriesViaDelegate(
          desc.layer_uid, desc.series_id.c_str(), del, warnings);
    }
  catch(exception &exc)
    {
    ReportNonLethalException(this, exc, "Image IO Error",
                             QString("Failed to load overlay image %1").arg(action->text()));
    }

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


void MainImageWindow::onRightDockDialogFinished()
{
  // Make the dock containing the wizard visible
  m_DockRight->setVisible(false);

  // Auto-adjust the canvas size
  QTimer::singleShot(0, this, SLOT(UpdateCanvasDimensions()));
  // this->UpdateCanvasDimensions();
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

#include <QKeyEvent>
bool MainImageWindow::event(QEvent *event)
{
    /*
  if(dynamic_cast<QKeyEvent *>(event))
    {
    QKeyEvent *kevent = dynamic_cast<QKeyEvent *>(event);
    std::cout << "KEY event: " << kevent->text().toStdString() << std::endl;
    std::cout << "  "
    std::cout << "MODIFIERS: " << (int) kevent->modifiers() << std::endl;
    }
    */
  return QWidget::event(event);
}

void MainImageWindow::on_actionOpenMain_triggered()
{
  // Prompt for unsaved changes
  if(!SaveModifiedLayersDialog::PromptForUnsavedChanges(m_Model))
    return;

  // Create a model for IO
  SmartPtr<LoadMainImageDelegate> delegate = LoadMainImageDelegate::New();
  delegate->Initialize(m_Model->GetDriver());
  SmartPtr<ImageIOWizardModel> model = ImageIOWizardModel::New();
  model->InitializeForLoad(m_Model, delegate);

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
  model->InitializeForLoad(m_Model, delegate);

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
  QString fuser = ShowSimpleSaveDialogWithHistory(
        this, m_Model, "Snapshots",
        "Save Snapshot - ITK-SNAP",
        "Snapshot File:",
        "PNG Image (*.png);;TIFF Image (*.tiff *.tif);;JPEG Image (*.jpg *.jpeg)",
        true,
        from_utf8(finput));

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
    target->update();
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
        this, m_Model, "LabelDescriptions",
        "Open Label Descriptions - ITK-SNAP",
        "Label Description File",
        "Text Files (*.txt);; Label Files (*.label)");

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
        this, m_Model, "LabelDescriptions",
        "Save Label Descriptions - ITK-SNAP",
        "Label Description File",
        "Text Files (*.txt);; Label Files (*.label)",
        true);

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
  // propagate the attributes.
  dialog->setAttribute(Qt::WA_PaintOnScreen, this->testAttribute(Qt::WA_PaintOnScreen));

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
  if(m_LayerInspector->isVisible() && m_LayerInspector->isActiveWindow())
    m_LayerInspector->advanceTab();
  else
    RaiseDialog(m_LayerInspector);
}

void MainImageWindow::on_actionAbout_triggered()
{
  // Show the about window
  RaiseDialog(m_AboutDialog);
}





void MainImageWindow::on_actionClear_triggered()
{
  IRISApplication *app = m_Model->GetDriver();

  // In snake mode, it is possible to clear the segmentation, but we don't
  // need to prompt for save
  if(app->IsSnakeModeActive())
    {
    app->ResetSNAPSegmentationImage();
    }
  else
    {
    // Prompt for unsaved changes
    if(!SaveModifiedLayersDialog::PromptForUnsavedSegmentationChanges(m_Model))
      return;

    app->ResetIRISSegmentationImage();
    }
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
        this, m_Model, "Project", "Open Workspace",
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
       m_Model, ALL_ROLES,
       SaveModifiedLayersDialog::DiscardDisabled
       | SaveModifiedLayersDialog::ProjectsDisabled))
    return false;

  // Use the global method
  return ::SaveWorkspace(this, m_Model, interactive, this);
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

  // Open a file browser and have the user select something
  std::string fuser = to_utf8(ShowSimpleSaveDialogWithHistory(
        this, m_Model, "Slices",
        "Save Slice - ITK-SNAP",
        "Slice Image File",
        "PNG Image (*.png);;TIFF Image (*.tiff *.tif);;JPEG Image (*.jpg *.jpeg)",true));

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
  // Launch a new SNAP in the relevant directory
  std::list<std::string> args;
  args.push_back("--cwd");
  args.push_back(to_utf8(GetFileDialogPath(m_Model, "MainImage")));
  m_Model->GetSystemInterface()->LaunchChildSNAPSimple(args);
}

void MainImageWindow::on_actionUnload_All_Overlays_triggered()
{
  // Prompt for changes to the overlays
  if(SaveModifiedLayersDialog::PromptForUnsavedChanges(m_Model, OVERLAY_ROLE))
    {
    m_Model->GetDriver()->UnloadAllOverlays();
    }
}

void MainImageWindow::changeEvent(QEvent *)
{
  if(m_Model)
    m_Model->GetSynchronizationModel()->SetCanBroadcast(this->isActiveWindow());
}

void MainImageWindow::on_actionClose_Window_triggered()
{
  // If main window is not the active window, close the active window instead of closing
  // any images. This is because Ctrl-W shortcut should be reserved for closign windows
  if(QApplication::activeWindow() != this)
    {
    QApplication::activeWindow()->close();
    return;
    }
}

void MainImageWindow::onRightDockCurrentChanged(int)
{
  // Adjust the width of the stack to match the current widget
  m_RightDockStack->setMaximumWidth(
        m_RightDockStack->currentWidget()->maximumWidth());
}


void MainImageWindow::on_actionUnload_Last_Overlay_triggered()
{
  // Get the selected ID
  unsigned long id_selected = m_Model->GetDriver()->GetGlobalState()->GetSelectedLayerId();

  // Find the actual layer
  ImageWrapperBase *layer = m_Model->GetDriver()->GetCurrentImageData()->FindLayer(id_selected, false);

  // Is this layer an overlay or a main image
  if(layer && layer == m_Model->GetDriver()->GetCurrentImageData()->GetMain())
    {
    // Prompt for unsaved changes
    if(SaveModifiedLayersDialog::PromptForUnsavedChanges(m_Model))
      {
      // Unload the main image
      m_Model->GetDriver()->UnloadMainImage();
      }
    }
  else
    {
    if(SaveModifiedLayersDialog::PromptForUnsavedChanges(m_Model, OVERLAY_ROLE))
      {
      m_Model->GetDriver()->UnloadOverlay(layer);
      }
    }
}


void MainImageWindow::on_actionToggleLayerLayout_triggered()
{
  m_Model->GetDisplayLayoutModel()->ToggleSliceViewLayerLayout();
}

void MainImageWindow::on_actionActivateNextLayer_triggered()
{
  m_Model->GetDisplayLayoutModel()->ActivateNextLayerInTiledMode();
}

void MainImageWindow::on_actionActivatePreviousLayer_triggered()
{
  m_Model->GetDisplayLayoutModel()->ActivatePrevLayerInTiledMode();
}

void MainImageWindow::on_actionInterpolate_Labels_triggered()
{
  RaiseDialog(m_InterpolateLabelsDialog);
}

void MainImageWindow::on_actionRegistration_triggered()
{
  // Remember the size of the window before the right dock was shown
  m_SizeWithoutRightDock = this->size();

  m_DockRight->setWindowTitle("Registration");
  m_RightDockStack->setCurrentWidget(m_RegistrationDialog);
  m_DockRight->setVisible(true);
}


void MainImageWindow::on_actionMainControlPanel_triggered()
{
  if(ui->actionMainControlPanel->isChecked())
    m_DockLeft->show();
  else
    m_DockLeft->hide();
}
