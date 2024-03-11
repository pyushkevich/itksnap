#include "SliceViewPanel.h"
#include "ui_SliceViewPanel.h"

#include "GlobalUIModel.h"
#include "SNAPEvents.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "itkCommand.h"
#include "CrosshairsRenderer.h"
#include "PolygonDrawingRenderer.h"
#include "DeformationGridRenderer.h"
#include "PaintbrushRenderer.h"
#include "SnakeROIRenderer.h"
#include "SnakeROIModel.h"
#include "SliceWindowCoordinator.h"
#include "PolygonDrawingModel.h"
#include "AnnotationModel.h"
#include "InteractiveRegistrationModel.h"
#include "AnnotationRenderer.h"
#include "QtWidgetActivator.h"
#include "SnakeModeRenderer.h"
#include "SnakeWizardModel.h"
#include "DisplayLayoutModel.h"
#include "PaintbrushModel.h"
#include "SliceWindowDecorationRenderer.h"
#include "LayerInspectorDialog.h"
#include "MainImageWindow.h"
#include "RegistrationRenderer.h"
#include "SNAPQtCommon.h"
#include "QtScrollbarCoupling.h"
#include "QtSliderCoupling.h"
#include "QtLabelCoupling.h"
#include <QCursor>
#include <QBitmap>
#include <QToolButton>
#include "AnnotationEditDialog.h"
#include <vtkRenderWindow.h>

#include "CrosshairsInteractionMode.h"
#include "ThumbnailInteractionMode.h"
#include "PolygonDrawingInteractionMode.h"
#include "SnakeROIInteractionMode.h"
#include "PaintbrushInteractionMode.h"
#include "AnnotationInteractionMode.h"
#include "RegistrationInteractionMode.h"

#include <QStackedLayout>
#include <QMenu>

SliceViewPanel::SliceViewPanel(QWidget *parent) :
    SNAPComponent(parent),
    ui(new Ui::SliceViewPanel)
{
  ui->setupUi(this);

  // Initialize
  m_GlobalUI = NULL;
  m_SliceModel = NULL;

  // Create my own renderers
  m_SnakeModeRenderer = SnakeModeRenderer::New();
  m_DecorationRenderer = SliceWindowDecorationRenderer::New();
  m_DeformationGridRenderer = DeformationGridRenderer::New();

  // Create the interaction modes
  m_CrosshairsMode = new CrosshairsInteractionMode(ui->sliceView);
  m_ZoomPanMode = new CrosshairsInteractionMode(ui->sliceView);
  m_ThumbnailMode = new ThumbnailInteractionMode(ui->sliceView);
  m_PolygonMode = new PolygonDrawingInteractionMode(ui->sliceView);
  m_SnakeROIMode = new SnakeROIInteractionMode(ui->sliceView);
  m_PaintbrushMode = new PaintbrushInteractionMode(ui->sliceView);
  m_AnnotationMode = new AnnotationInteractionMode(ui->sliceView);
  m_RegistrationMode = new RegistrationInteractionMode(ui->sliceView);

  // Add the interaction modes to the main slice view's layout
  ui->sliceView->layout()->addWidget(m_CrosshairsMode);
  ui->sliceView->layout()->addWidget(m_ZoomPanMode);
  ui->sliceView->layout()->addWidget(m_ThumbnailMode);
  ui->sliceView->layout()->addWidget(m_PolygonMode);
  ui->sliceView->layout()->addWidget(m_SnakeROIMode);
  ui->sliceView->layout()->addWidget(m_PaintbrushMode);
  ui->sliceView->layout()->addWidget(m_AnnotationMode);
  ui->sliceView->layout()->addWidget(m_RegistrationMode);

  QString menuStyle = "font-size: 12px;";

  // Create the popup menus for the polygon mode
  m_MenuPolyInactive = new QMenu(this->m_PolygonMode);
  m_MenuPolyInactive->setStyleSheet(menuStyle);
  m_MenuPolyInactive->addAction(ui->actionPaste);

  m_MenuPolyDrawing = new QMenu(this->m_PolygonMode);
  m_MenuPolyDrawing->setStyleSheet(menuStyle);
  m_MenuPolyDrawing->addAction(ui->actionComplete);
  m_MenuPolyDrawing->addAction(ui->actionCompleteAndAccept);
  m_MenuPolyDrawing->addAction(ui->actionUndo);
  m_MenuPolyDrawing->addAction(ui->actionClearDrawing);

  m_MenuPolyEditing = new QMenu(this->m_PolygonMode);
  m_MenuPolyEditing->setStyleSheet(menuStyle);
  m_MenuPolyEditing->addAction(ui->actionAccept);
  m_MenuPolyEditing->addAction(ui->actionDeleteSelected);
  m_MenuPolyEditing->addAction(ui->actionSplitSelected);
  m_MenuPolyEditing->addAction(ui->actionClearPolygon);

  // Connect the actions to the toolbar buttons (sucks to do this by hand)
  ui->btnAcceptPolygon->setDefaultAction(ui->actionAccept);
  ui->btnPastePolygon->setDefaultAction(ui->actionPaste);
  ui->btnClearDrawing->setDefaultAction(ui->actionClearDrawing);
  ui->btnCloseLoop->setDefaultAction(ui->actionComplete);
  ui->btnDeleteNodes->setDefaultAction(ui->actionDeleteSelected);
  ui->btnDeletePolygon->setDefaultAction(ui->actionClearPolygon);
  ui->btnSplitNodes->setDefaultAction(ui->actionSplitSelected);
  ui->btnUndoLast->setDefaultAction(ui->actionUndo);

  // And also connect to the corresponding slots
  connect(ui->actionAccept, &QAction::triggered, m_PolygonMode, &PolygonDrawingInteractionMode::onAcceptPolygon);
  connect(ui->actionPaste, &QAction::triggered, m_PolygonMode, &PolygonDrawingInteractionMode::onPastePolygon);
  connect(ui->actionClearDrawing, &QAction::triggered, m_PolygonMode, &PolygonDrawingInteractionMode::onCancelDrawing);
  connect(ui->actionComplete, &QAction::triggered, m_PolygonMode, &PolygonDrawingInteractionMode::onCloseLoopAndEdit);
  connect(ui->actionDeleteSelected, &QAction::triggered, m_PolygonMode, &PolygonDrawingInteractionMode::onDeleteSelected);
  connect(ui->actionClearPolygon, &QAction::triggered, m_PolygonMode, &PolygonDrawingInteractionMode::onClearPolygon);
  connect(ui->actionSplitSelected, &QAction::triggered, m_PolygonMode, &PolygonDrawingInteractionMode::onSplitSelected);
  connect(ui->actionUndo, &QAction::triggered, m_PolygonMode, &PolygonDrawingInteractionMode::onUndoLastPoint);


  ui->btnAnnotationAcceptLine->setDefaultAction(ui->actionAnnotationAcceptLine);
  ui->btnAnnotationClearLine->setDefaultAction(ui->actionAnnotationClearLine);
  ui->btnAnnotationSelectAll->setDefaultAction(ui->actionAnnotationSelectAll);
  ui->btnAnnotationEdit->setDefaultAction(ui->actionAnnotationEdit);
  ui->btnAnnotationDeleteSelected->setDefaultAction(ui->actionAnnotationDelete);
  ui->btnAnnotationNext->setDefaultAction(ui->actionAnnotationNext);
  ui->btnAnnotationPrevious->setDefaultAction(ui->actionAnnotationPrevious);

  this->addAction(ui->actionZoom_In);
  this->addAction(ui->actionZoom_Out);

  // Connect the context menu signal from polygon mode to this widget
  connect(this->m_PolygonMode, SIGNAL(contextMenuRequested()), SLOT(onContextMenu()));

  // Arrange the interaction modes into a tree structure. The first child of
  // every interaction mode is an empty QWidget. The tree is used to allow
  // events to fall through from one interaction mode to another

  // TODO: can we think of a more dynamic system for doing this? Essentially,
  // there is a pipeline through which events pass through, which is
  // reconfigured when the mode changes.


  // Add all the individual interactors to the main panel using a stacked
  // layout. This assures that all of the interactors have the same size as
  // the sliceView. The actual propagation of events is handled by the
  // event filter logic.
  /*
  QStackedLayout *loMain = new QStackedLayout();
  loMain->setContentsMargins(0,0,0,0);
  loMain->addWidget(this->m_CrosshairsMode);
  loMain->addWidget(this->m_ZoomPanMode);
  loMain->addWidget(this->m_ThumbnailMode);
  loMain->addWidget(this->m_PolygonMode);
  loMain->addWidget(this->m_SnakeROIMode);
  loMain->addWidget(this->m_PaintbrushMode);
  delete ui->sliceView->layout();
  ui->sliceView->setLayout(loMain);
  */

  // Configure the initial event chain
  m_CurrentEventFilter = NULL;
  ConfigureEventChain(this->m_CrosshairsMode);

  // Also lay out the pages
  QStackedLayout *loPages = new QStackedLayout();
  loPages->addWidget(ui->pageDefault);
  loPages->addWidget(ui->pagePolygonDraw);
  loPages->addWidget(ui->pagePolygonEdit);
  loPages->addWidget(ui->pagePolygonInactive);
  loPages->addWidget(ui->pageAnnotateLineActive);
  loPages->addWidget(ui->pageAnnotateSelect);
  delete ui->toolbar->layout();
  ui->toolbar->setLayout(loPages);

  // Send wheel events from Crosshairs mode to the slider
  this->m_CrosshairsMode->SetWheelEventTargetWidget(ui->inSlicePosition);

  // Set page size on the slice position widget
  ui->inSlicePosition->setPageStep(5);

  // Set up the drawing cursor
  QBitmap bmBitmap(":/root/crosshair_cursor_bitmap.png");
  QBitmap bmMask(":/root/crosshair_cursor_mask.png");
  m_DrawingCrosshairCursor = new QCursor(bmBitmap, bmMask, 7, 7);

  // Configure the context tool button
  m_ContextToolButton = new QToolButton(ui->sliceView->GetInternalWidget());
  m_ContextToolButton->setIcon(QIcon(":/root/context_gray_10.png"));
  m_ContextToolButton->setVisible(false);
  m_ContextToolButton->setAutoRaise(true);
  m_ContextToolButton->setIconSize(QSize(10,10));
  m_ContextToolButton->setMinimumSize(QSize(16,16));
  m_ContextToolButton->setMaximumSize(QSize(16,16));
  m_ContextToolButton->setPopupMode(QToolButton::InstantPopup);
  m_ContextToolButton->setStyleSheet("QToolButton::menu-indicator { image: none; }");
}

SliceViewPanel::~SliceViewPanel()
{
  delete ui;
  delete m_DrawingCrosshairCursor;
}

GenericSliceView * SliceViewPanel::GetSliceView()
{
  return ui->sliceView;
}

void SliceViewPanel::Initialize(GlobalUIModel *model, unsigned int index)
{
  // Store the model
  this->m_GlobalUI = model;
  this->m_Index = index;

  // Get the slice model
  m_SliceModel = m_GlobalUI->GetSliceModel(index);

  // Initialize the slice view
  ui->sliceView->SetModel(m_SliceModel);

  // Initialize the interaction modes
  this->m_CrosshairsMode->SetModel(m_GlobalUI->GetCursorNavigationModel(index));
  this->m_ZoomPanMode->SetModel(m_GlobalUI->GetCursorNavigationModel(index));
  this->m_ZoomPanMode->SetMouseButtonBehaviorToZoomPanMode();
  this->m_ThumbnailMode->SetModel(m_GlobalUI->GetCursorNavigationModel(index));
  this->m_PolygonMode->SetModel(m_GlobalUI->GetPolygonDrawingModel(index));
  this->m_SnakeROIMode->SetModel(m_GlobalUI->GetSnakeROIModel(index));
  this->m_PaintbrushMode->SetModel(m_GlobalUI->GetPaintbrushModel(index));
  this->m_AnnotationMode->SetModel(m_GlobalUI->GetAnnotationModel(index));
  this->m_RegistrationMode->SetModel(m_GlobalUI->GetInteractiveRegistrationModel(index));

  // ui->labelQuickList->SetModel(m_GlobalUI);

  // Initialize the 'orphan' renderers (without a custom widget)
  GenericSliceRenderer *parentRenderer =
      static_cast<GenericSliceRenderer *>(ui->sliceView->GetRenderer());

  m_DecorationRenderer->SetParentRenderer(parentRenderer);
  m_DecorationRenderer->SetModel(m_SliceModel);
  m_SnakeModeRenderer->SetParentRenderer(parentRenderer);
  m_SnakeModeRenderer->SetModel(m_GlobalUI->GetSnakeWizardModel());
  m_DeformationGridRenderer->SetParentRenderer(parentRenderer);
  m_DeformationGridRenderer->SetModel(m_SliceModel->GetDeformationGridModel());

  // Listen to cursor change events, which require a repaint of the slice view
  connectITK(m_GlobalUI->GetDriver(), CursorUpdateEvent());
  connectITK(m_GlobalUI->GetDriver(), MainImageDimensionsChangeEvent());

  // Add listener for changes to the model
  connectITK(m_SliceModel, ModelUpdateEvent());

  // Listen to toolbar change events
  connectITK(m_GlobalUI, ToolbarModeChangeEvent());

  // Listen to polygon and annotation model state change events
  connectITK(m_GlobalUI->GetPolygonDrawingModel(index), StateMachineChangeEvent());
  connectITK(m_GlobalUI->GetAnnotationModel(index), StateMachineChangeEvent());

  // Listen to the Snake ROI model too
  connectITK(m_GlobalUI->GetSnakeROIModel(index),
             ModelUpdateEvent());

  // Listen to paintbrush motion
  connectITK(m_GlobalUI->GetPaintbrushModel(index),
             PaintbrushModel::PaintbrushMovedEvent());

  // Listen to annotation changes
  connectITK(m_GlobalUI->GetAnnotationModel(index), ModelUpdateEvent());

  // Listen to registration changes
  connectITK(m_GlobalUI->GetInteractiveRegistrationModel(index), ModelUpdateEvent());

  // Listen to all (?) events from the snake wizard as well
  connectITK(m_GlobalUI->GetSnakeWizardModel(), IRISEvent());

  // Widget coupling
  makeCoupling(ui->inSlicePosition, m_SliceModel->GetSliceIndexModel());  
  makeCoupling(ui->lblSliceInfo, m_SliceModel->GetSliceIndexTextModel());

  // Activation
  activateOnFlag(this, m_GlobalUI, UIF_BASEIMG_LOADED);

  // Activation for the tile/thumb controls
  activateOnFlag(ui->btnToggleLayout, m_GlobalUI, UIF_MULTIPLE_BASE_LAYERS,
                 QtWidgetActivator::HideInactive);

  // Set up activation for polygon buttons
  PolygonDrawingModel *pm = m_GlobalUI->GetPolygonDrawingModel(index);

  activateOnAllFlags(ui->actionAccept, pm, UIF_EDITING, UIF_HAVEPOLYGON);
  activateOnAllFlags(ui->actionPaste, pm, UIF_INACTIVE, UIF_HAVECACHED);
  activateOnAllFlags(ui->actionClearDrawing, pm, UIF_DRAWING, UIF_HAVEPOLYGON);
  activateOnAllFlags(ui->actionComplete, pm, UIF_DRAWING, UIF_HAVEPOLYGON);
  activateOnAllFlags(ui->actionCompleteAndAccept, pm, UIF_DRAWING, UIF_HAVEPOLYGON);
  activateOnAllFlags(ui->actionDeleteSelected, pm, UIF_EDITING, UIF_HAVE_VERTEX_SELECTION);
  activateOnAllFlags(ui->actionSplitSelected, pm, UIF_EDITING, UIF_HAVE_EDGE_SELECTION);
  activateOnAllFlags(ui->actionUndo, pm, UIF_DRAWING, UIF_HAVEPOLYGON);
  activateOnAllFlags(ui->actionClearPolygon, pm, UIF_EDITING, UIF_HAVEPOLYGON);

  // Set up activation from the annotation model
  AnnotationModel *am = m_GlobalUI->GetAnnotationModel(index);

  activateOnFlag(ui->actionAnnotationAcceptLine, am, AnnotationModel::UIF_LINE_MODE_DRAWING);
  activateOnFlag(ui->actionAnnotationClearLine, am, AnnotationModel::UIF_LINE_MODE_DRAWING);
  activateOnFlag(ui->actionAnnotationEdit, am, AnnotationModel::UIF_SELECTION_SINGLE);
  activateOnFlag(ui->actionAnnotationDelete, am, AnnotationModel::UIF_SELECTION_ANY);
  activateOnFlag(ui->actionAnnotationNext, am, AnnotationModel::UIF_ANNOTATIONS_EXIST);
  activateOnFlag(ui->actionAnnotationPrevious, am, AnnotationModel::UIF_ANNOTATIONS_EXIST);
  activateOnFlag(ui->actionAnnotationPrevious, am, AnnotationModel::UIF_ANNOTATIONS_EXIST);

  // Update the expand view button
  this->UpdateExpandViewButton();

  // Listen to the events affecting the expand view button
  DisplayLayoutModel *dlm = m_GlobalUI->GetDisplayLayoutModel();
  connectITK(dlm, DisplayLayoutModel::ViewPanelLayoutChangeEvent());
  connectITK(dlm, DisplayLayoutModel::LayerLayoutChangeEvent());

  // Arrange the rendering overlays and widgets based on current mode
  this->OnToolbarModeChange();

  // Listen for hover changes to move and activate widgets
  connectITK(m_SliceModel->GetHoveredImageLayerIdModel(), ValueChangedEvent(),
             SLOT(OnHoveredLayerChange(const EventBucket &)));

  connectITK(m_SliceModel->GetHoveredImageIsThumbnailModel(), ValueChangedEvent(),
             SLOT(OnHoveredLayerChange(const EventBucket &)));
}

void SliceViewPanel::onModelUpdate(const EventBucket &eb)
{
  if(eb.HasEvent(ToolbarModeChangeEvent()) ||
     eb.HasEvent(StateMachineChangeEvent()))
    {
    OnToolbarModeChange();
    }
  if(eb.HasEvent(DisplayLayoutModel::ViewPanelLayoutChangeEvent()) ||
     eb.HasEvent(DisplayLayoutModel::LayerLayoutChangeEvent()))
    {
    UpdateExpandViewButton();
    }

  // this is causing crash on Linux
  // ui->sliceView->GetRenderWindow()->Render();
  ui->sliceView->update();
}

void SliceViewPanel::ConfigureEventChain(QWidget *w)
{
  // Get the internal widget that gets the actual events
  QWidget *sviw = ui->sliceView->GetInternalWidget();

  // Remove all event filters from the slice view
  QObjectList kids = ui->sliceView->children();
  for(QObjectList::Iterator it = kids.begin(); it!=kids.end(); ++it)
    if(*it != sviw)
      sviw->removeEventFilter(*it);

  // Now add the event filters in the order in which we want them to react
  // to events. The last event filter is first to receive events, and should
  // thus be the thumbnail interaction mode. The first event filter is always
  // the crosshairs interaction mode, which is the fallback for all others.
  sviw->installEventFilter(this->m_CrosshairsMode);

  // If the current mode is not crosshairs mode, add it as the filter
  if(w != this->m_CrosshairsMode)
    {
    sviw->installEventFilter(w);
    }

  // The last guy in the chain is the thumbnail interactor
  sviw->installEventFilter(this->m_ThumbnailMode);
}

// TODO: implement semi-transparent rendering on widgets on top of the
// OpenGL scene using code from
// http://www.qtcentre.org/wiki/index.php?title=Accelerate_your_Widgets_with_OpenGL
void SliceViewPanel::enterEvent(QEvent *)
{
  /*
  ui->mainToolbar->show();
  ui->sidebar->show();
  */
}

void SliceViewPanel::leaveEvent(QEvent *)
{
  /*
  ui->mainToolbar->hide();
  ui->sidebar->hide();
  */


}

void SliceViewPanel::SetActiveMode(QWidget *mode, bool clearChildren)
{
  // If the widget does not have a stacked layout, do nothing, we've reached
  // the end of the recursion
  QStackedLayout *loParent =
      dynamic_cast<QStackedLayout *>(mode->parentWidget()->layout());

  if(loParent)
    {
    // Set the mode as the current widget in the parent
    loParent->setCurrentWidget(mode);

    // Make sure the parent widget is also the current widget
    SetActiveMode(mode->parentWidget(), false);

    // Make sure no children are selected
    if(clearChildren)
      {
      // If the selected mode has child modes, make sure none is selected
      QStackedLayout *lo = dynamic_cast<QStackedLayout *>(mode->layout());
      if(lo)
        lo->setCurrentIndex(0);
      }
    }
}

void SliceViewPanel::OnToolbarModeChange()
{
  // Get the renderer and configure its overlays
  GenericSliceRenderer *ren = (GenericSliceRenderer *) ui->sliceView->GetRenderer();

  // Configure the renderers
  GenericSliceRenderer::RendererDelegateList delegates;

  // Append the overlays in the right order
  delegates.clear();
  delegates.push_back(m_SnakeModeRenderer);
  delegates.push_back(this->m_CrosshairsMode->GetRenderer());
  delegates.push_back(this->m_AnnotationMode->GetRenderer());
  delegates.push_back(this->m_PolygonMode->GetRenderer());

  // ovGlobal.clear();

  switch(m_GlobalUI->GetGlobalState()->GetToolbarMode())
    {
    case POLYGON_DRAWING_MODE:
      ConfigureEventChain(this->m_PolygonMode);
      break;
    case PAINTBRUSH_MODE:
      ConfigureEventChain(this->m_PaintbrushMode);
      delegates.push_back(this->m_PaintbrushMode->GetRenderer());
      break;
    case ANNOTATION_MODE:
      ConfigureEventChain(this->m_AnnotationMode);
      delegates.push_back(this->m_AnnotationMode->GetRenderer());
      break;
    case REGISTRATION_MODE:
      ConfigureEventChain(this->m_RegistrationMode);
      delegates.push_back(this->m_RegistrationMode->GetRenderer());
      break;
    case ROI_MODE:
      ConfigureEventChain(this->m_SnakeROIMode);
      delegates.push_back(this->m_SnakeROIMode->GetRenderer());
      break;
    case CROSSHAIRS_MODE:
      ConfigureEventChain(this->m_CrosshairsMode);
      break;
    case NAVIGATION_MODE:
      ConfigureEventChain(this->m_ZoomPanMode);
      break;
    }

  delegates.push_back(m_DecorationRenderer);
  delegates.push_back(m_DeformationGridRenderer);

  // Set the overlays
  ren->SetDelegates(delegates);
  // ren->SetGlobalOverlays(ovTiled);

  // Need to change to the appropriate page
  QStackedLayout *loPages =
      static_cast<QStackedLayout *>(ui->toolbar->layout());
  if(m_GlobalUI->GetGlobalState()->GetToolbarMode() == POLYGON_DRAWING_MODE)
    {
    switch(m_GlobalUI->GetPolygonDrawingModel(m_Index)->GetState())
      {
      case PolygonDrawingModel::DRAWING_STATE:
        loPages->setCurrentWidget(ui->pagePolygonDraw); break;
      case PolygonDrawingModel::EDITING_STATE:
        loPages->setCurrentWidget(ui->pagePolygonEdit); break;
      case PolygonDrawingModel::INACTIVE_STATE:
        loPages->setCurrentWidget(ui->pagePolygonInactive); break;
      }
    }
  else if(m_GlobalUI->GetGlobalState()->GetToolbarMode() == ANNOTATION_MODE)
    {
    AnnotationModel *am = m_GlobalUI->GetAnnotationModel(m_Index);
    if(am->GetFlagDrawingLine())
      loPages->setCurrentWidget(ui->pageAnnotateLineActive);
    else if(am->GetAnnotationMode() == ANNOTATION_SELECT)
      loPages->setCurrentWidget(ui->pageAnnotateSelect);
    else
      loPages->setCurrentWidget(ui->pageDefault);
    }
  else
    {
    loPages->setCurrentWidget(ui->pageDefault);
    }
}

void SliceViewPanel::on_btnZoomToFit_clicked()
{
  m_GlobalUI->GetSliceCoordinator()->ResetViewToFitInOneWindow(m_Index);

}

void SliceViewPanel::onContextMenu()
{
  if(m_GlobalUI->GetGlobalState()->GetToolbarMode() == POLYGON_DRAWING_MODE)
    {
    QMenu *menu = NULL;
    switch(m_GlobalUI->GetPolygonDrawingModel(m_Index)->GetState())
      {
      case PolygonDrawingModel::DRAWING_STATE:
        menu = m_MenuPolyDrawing; break;
      case PolygonDrawingModel::EDITING_STATE:
        menu = m_MenuPolyEditing; break;
      case PolygonDrawingModel::INACTIVE_STATE:
        menu = m_MenuPolyInactive; break;
      }

    if(menu)
      {
      menu->popup(QCursor::pos());
      }
    }
}

void SliceViewPanel::SetMouseMotionTracking(bool enable)
{
  ui->sliceView->setMouseTracking(enable);
  // TODO: in the future, consider using a better cursor for polygon drawing operations
  /*
  if(enable)
    this->setCursor(*m_DrawingCrosshairCursor);
  else
    this->setCursor(QCursor(Qt::ArrowCursor));
    */
}

void SliceViewPanel::on_btnExpand_clicked()
{
  // Get the layout applied when the button is pressed
  DisplayLayoutModel *dlm = m_GlobalUI->GetDisplayLayoutModel();
  DisplayLayoutModel::ViewPanelLayout layout =
      dlm->GetViewPanelExpandButtonActionModel(m_Index)->GetValue();

  // Apply this layout
  dlm->GetViewPanelLayoutModel()->SetValue(layout);
}

void SliceViewPanel::UpdateExpandViewButton()
{
  // Get the layout applied when the button is pressed
  DisplayLayoutModel *dlm = m_GlobalUI->GetDisplayLayoutModel();
  DisplayLayoutModel::ViewPanelLayout layout =
      dlm->GetViewPanelExpandButtonActionModel(m_Index)->GetValue();

  // Set the appropriate icon
  static const char* iconNames[] =
    { "fourviews", "axial", "coronal", "sagittal", "3d" };

  QString iconFile = QString(":/root/dl_%1.png").arg(iconNames[layout]);
  ui->btnExpand->setIcon(QIcon(iconFile));

  // Set the tooltip
  if(layout == DisplayLayoutModel::VIEW_ALL)
    {
    ui->btnExpand->setToolTip("Restore the four-panel display configuration");
    }
  else
    {
    ui->btnExpand->setToolTip("Expand this view to occupy the entire window");
    }

  // Also expand the tile/cascade button
  LayerLayout ll = dlm->GetSliceViewLayerLayoutModel()->GetValue();
  if(ll == LAYOUT_TILED)
    {
    ui->btnToggleLayout->setIcon(QIcon(":/root/layout_thumb_16.png"));
    }
  else if(ll == LAYOUT_STACKED)
    {
    ui->btnToggleLayout->setIcon(QIcon(":/root/layout_tile_16.png"));
    }
}


void SliceViewPanel::on_btnScreenshot_clicked()
{
  MainImageWindow *parent = findParentWidget<MainImageWindow>(this);
  parent->ExportScreenshot(m_Index);
}

void SliceViewPanel::on_btnToggleLayout_clicked()
{
  m_GlobalUI->GetDisplayLayoutModel()->ToggleSliceViewLayerLayout();
}

void SliceViewPanel::on_actionZoom_In_triggered()
{
  // Zoom in
  m_GlobalUI->GetSliceCoordinator()->ZoomInOrOutInOneWindow(m_Index, 1.1);
}

void SliceViewPanel::on_actionZoom_Out_triggered()
{
  // Zoom out
  m_GlobalUI->GetSliceCoordinator()->ZoomInOrOutInOneWindow(m_Index, 1.0 / 1.1);
}

void SliceViewPanel::OnHoveredLayerChange(const EventBucket &eb)
{
  // Determine the position where to draw the button
  const SliceViewportLayout::SubViewport *vp = m_SliceModel->GetHoveredViewport();
  if(vp)
    {
    // Set up the location of the button
    int vppr = m_SliceModel->GetSizeReporter()->GetViewportPixelRatio();
    int x = (vp->pos[0] + vp->size[0]) / vppr - m_ContextToolButton->width();
    int y = (ui->sliceView->height() - 1) - (vp->pos[1] + vp->size[1]) / vppr;
    m_ContextToolButton->setVisible(true);
    m_ContextToolButton->move(x, y);

    // Set up the context menu on the button
    MainImageWindow *winmain = findParentWidget<MainImageWindow>(this);
    LayerInspectorDialog *inspector = winmain->GetLayerInspector();

    // Get the corresponding context menu
    QMenu *menu = inspector->GetLayerContextMenu(
                    m_SliceModel->GetDriver()->GetCurrentImageData()->FindLayer(
                      m_SliceModel->GetHoveredImageLayerId(), false, ALL_ROLES));

    // Show the menu
    m_ContextToolButton->setMenu(menu);
    m_ContextToolButton->setDown(false);
    }
  else
    {
    m_ContextToolButton->setVisible(false);
    m_ContextToolButton->setMenu(NULL);
    m_ContextToolButton->setDown(false);
    }
}







void SliceViewPanel::on_actionAnnotationAcceptLine_triggered()
{
  m_GlobalUI->GetAnnotationModel(m_Index)->AcceptLine();
}

void SliceViewPanel::on_actionAnnotationClearLine_triggered()
{
  m_GlobalUI->GetAnnotationModel(m_Index)->CancelLine();
}

void SliceViewPanel::on_actionAnnotationSelectAll_triggered()
{
  m_GlobalUI->GetAnnotationModel(m_Index)->SelectAllOnSlice();
}

void SliceViewPanel::on_actionAnnotationDelete_triggered()
{
  m_GlobalUI->GetAnnotationModel(m_Index)->DeleteSelectedOnSlice();
}

void SliceViewPanel::on_actionAnnotationNext_triggered()
{
  m_GlobalUI->GetAnnotationModel(m_Index)->GoToNextAnnotation();
}

void SliceViewPanel::on_actionAnnotationPrevious_triggered()
{
  m_GlobalUI->GetAnnotationModel(m_Index)->GoToPreviousAnnotation();
}

void SliceViewPanel::on_actionAnnotationEdit_triggered()
{
  // Show the annotation editor
  AnnotationEditDialog *dialog = new AnnotationEditDialog(this);
  dialog->SetModel(m_GlobalUI->GetAnnotationModel(m_Index));
  dialog->show();
  dialog->activateWindow();
  dialog->raise();
}
