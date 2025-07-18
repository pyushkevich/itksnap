#include "SliceViewPanel.h"
#include "ui_SliceViewPanel.h"

#include "GlobalUIModel.h"
#include "SNAPEvents.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "CrosshairsRenderer.h"
#include "PolygonDrawingRenderer.h"
#include "DeformationGridRenderer.h"
#include "PaintbrushRenderer.h"
#include "SnakeROIRenderer.h"
#include "SnakeROIModel.h"
#include "SliceWindowCoordinator.h"
#include "PolygonDrawingModel.h"
#include "PolygonDrawingRenderer.h"
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

#include "GenericSliceRenderer.h"
#include "QtReporterDelegates.h"
#include <QStackedLayout>
#include <QMenu>

#include "QtFrameBufferOpenGLWidget.h"


/**
 * Chain of interaction mode delegates and render delegates for the slice view.
 * They are in their own class to keep the code a bit more tidy.
 */
class SliceViewDelegateChain
{
  friend class SliceViewPanel;

public:
  SliceViewDelegateChain(QWidget *sliceView, QWidget *canvasWidget, GenericSliceRenderer *mainRenderer)
    : m_SliceView(sliceView)
    , m_CanvasWidget(canvasWidget)
    , m_MainRenderer(mainRenderer)
  {
    // Create the interaction modes
    m_CrosshairsMode = new CrosshairsInteractionMode(sliceView, canvasWidget);
    m_ZoomPanMode = new CrosshairsInteractionMode(sliceView, canvasWidget);
    m_ThumbnailMode = new ThumbnailInteractionMode(sliceView, canvasWidget);
    m_PolygonMode = new PolygonDrawingInteractionMode(sliceView, canvasWidget);
    m_SnakeROIMode = new SnakeROIInteractionMode(sliceView, canvasWidget);
    m_PaintbrushMode = new PaintbrushInteractionMode(sliceView, canvasWidget);
    m_AnnotationMode = new AnnotationInteractionMode(sliceView, canvasWidget);
    m_RegistrationMode = new RegistrationInteractionMode(sliceView, canvasWidget);

    // Create the renderers
    m_CrosshairsRenderer = CrosshairsRenderer::New();
    m_DecorationRenderer = SliceWindowDecorationRenderer::New();
    m_PolygonDrawingRenderer = PolygonDrawingRenderer::New();
    m_PaintbrushRenderer = PaintbrushRenderer::New();
    m_AnnotationRenderer = AnnotationRenderer::New();
    m_DeformationGridRenderer = DeformationGridRenderer::New();
    m_SnakeModeRenderer = SnakeModeRenderer::New();
    m_SnakeROIRenderer = SnakeROIRenderer::New();
    m_RegistrationRenderer = RegistrationRenderer::New();

    // Configure mode map
    m_ToolbarModeMap[POLYGON_DRAWING_MODE] = std::make_pair(m_PolygonMode, nullptr);
    m_ToolbarModeMap[PAINTBRUSH_MODE] = std::make_pair(m_PaintbrushMode, m_PaintbrushRenderer);
    m_ToolbarModeMap[ANNOTATION_MODE] = std::make_pair(m_AnnotationMode, m_AnnotationRenderer);
    m_ToolbarModeMap[REGISTRATION_MODE] = std::make_pair(m_RegistrationMode, m_RegistrationRenderer);
    m_ToolbarModeMap[ROI_MODE] = std::make_pair(m_SnakeROIMode, m_SnakeROIRenderer);
    m_ToolbarModeMap[CROSSHAIRS_MODE] = std::make_pair(m_CrosshairsMode, nullptr);
    m_ToolbarModeMap[NAVIGATION_MODE] = std::make_pair(m_ZoomPanMode, nullptr);

    // Create list of all modes
    m_AllModes.push_back(m_ThumbnailMode);
    for (auto it : m_ToolbarModeMap)
      m_AllModes.push_back(it.second.first);

    // Add the interaction modes to the main slice view's layout
    for (auto *widget : m_AllModes)
      m_SliceView->layout()->addWidget(widget);

    // Configure default chain
    ConfigureEventChain(CROSSHAIRS_MODE);
  }

  virtual ~SliceViewDelegateChain() {}

  void ConfigureEventChain(ToolbarModeType mode)
  {
    auto &active_ir = m_ToolbarModeMap[mode];

    // Remove all event filters from the slice view
    for (auto *widget : m_AllModes)
      m_CanvasWidget->removeEventFilter(widget);

    // Now add the event filters in the order in which we want them to react
    // to events. The last event filter is first to receive events, and should
    // thus be the thumbnail interaction mode. The first event filter is always
    // the crosshairs interaction mode, which is the fallback for all others.
    m_CanvasWidget->installEventFilter(this->m_CrosshairsMode);

    // If the current mode is not crosshairs mode, add it as the filter
    if (active_ir.first != this->m_CrosshairsMode)
      m_CanvasWidget->installEventFilter(active_ir.first);

    // The last guy in the chain is the thumbnail interactor
    m_CanvasWidget->installEventFilter(this->m_ThumbnailMode);

    // Configure the renderers in desired order
    GenericSliceRenderer::RendererDelegateList renderer_delegates;

    // These renderers render below everything else
    renderer_delegates.push_back(m_SnakeModeRenderer);
    renderer_delegates.push_back(m_DeformationGridRenderer);
    renderer_delegates.push_back(m_CrosshairsRenderer);
    renderer_delegates.push_back(m_PolygonDrawingRenderer);

    // This is the mode-specific renderer
    if(active_ir.second)
      renderer_delegates.push_back(active_ir.second);

    // These renderers render on top of everything else
    renderer_delegates.push_back(m_DecorationRenderer);

    // Send the delegates
    m_MainRenderer->SetDelegates(renderer_delegates);
  }

  void SetModel(GlobalUIModel *model, unsigned int index)
  {
    // Initialize the interaction modes
    m_CrosshairsMode->SetModel(model->GetCursorNavigationModel(index));
    m_ZoomPanMode->SetModel(model->GetCursorNavigationModel(index));
    m_ZoomPanMode->SetMouseButtonBehaviorToZoomPanMode();
    m_ThumbnailMode->SetModel(model->GetCursorNavigationModel(index));
    m_PolygonMode->SetModel(model->GetPolygonDrawingModel(index));
    m_SnakeROIMode->SetModel(model->GetSnakeROIModel(index));
    m_PaintbrushMode->SetModel(model->GetPaintbrushModel(index));
    m_AnnotationMode->SetModel(model->GetAnnotationModel(index));
    m_RegistrationMode->SetModel(model->GetInteractiveRegistrationModel(index));

    // Initialize the renderers
    m_CrosshairsRenderer->SetModel(model->GetSliceModel(index));
    m_DecorationRenderer->SetModel(model->GetSliceModel(index));
    m_PaintbrushRenderer->SetModel(model->GetPaintbrushModel(index));
    m_PolygonDrawingRenderer->SetModel(model->GetPolygonDrawingModel(index));
    m_AnnotationRenderer->SetModel(model->GetAnnotationModel(index));
    m_DeformationGridRenderer->SetModel(model->GetSliceModel(index)->GetDeformationGridModel());
    m_SnakeModeRenderer->SetModel(model->GetSliceModel(index));
    m_SnakeROIRenderer->SetModel(model->GetSnakeROIModel(index));
    m_RegistrationRenderer->SetModel(model->GetInteractiveRegistrationModel(index));
  }

private:
  // Client widget, widget that actually is rendered on
  QWidget *m_SliceView, *m_CanvasWidget;

  // Renderer that does the main rendering and accepts delegates
  GenericSliceRenderer *m_MainRenderer;

  // Interaction modes
  CrosshairsInteractionMode     *m_CrosshairsMode, *m_ZoomPanMode;
  ThumbnailInteractionMode      *m_ThumbnailMode;
  PolygonDrawingInteractionMode *m_PolygonMode;
  SnakeROIInteractionMode       *m_SnakeROIMode;
  PaintbrushInteractionMode     *m_PaintbrushMode;
  AnnotationInteractionMode     *m_AnnotationMode;
  RegistrationInteractionMode   *m_RegistrationMode;

  // Renderers
  SmartPtr<CrosshairsRenderer>            m_CrosshairsRenderer;
  SmartPtr<PolygonDrawingRenderer>        m_PolygonDrawingRenderer;
  SmartPtr<PaintbrushRenderer>            m_PaintbrushRenderer;
  SmartPtr<AnnotationRenderer>            m_AnnotationRenderer;
  SmartPtr<DeformationGridRenderer>       m_DeformationGridRenderer;
  SmartPtr<SnakeModeRenderer>             m_SnakeModeRenderer;
  SmartPtr<SnakeROIRenderer>              m_SnakeROIRenderer;
  SmartPtr<RegistrationRenderer>          m_RegistrationRenderer;
  SmartPtr<SliceWindowDecorationRenderer> m_DecorationRenderer;

  // Map from toolbar mode to interaction mode
  using IRPair = std::pair<QWidget *, SliceRendererDelegate *>;
  std::map<ToolbarModeType, IRPair> m_ToolbarModeMap;

  // All modes including those that don't correspond to a toolbar mode
  std::list<QWidget *> m_AllModes;
};


SliceViewPanel::SliceViewPanel(QWidget *parent) :
    SNAPComponent(parent),
    ui(new Ui::SliceViewPanel)
{
  ui->setupUi(this);

  // Initialize
  m_GlobalUI = NULL;
  m_SliceModel = NULL;

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

  // Set page size on the slice position widget
  ui->inSlicePosition->setPageStep(5);

  // Set up the drawing cursor
  QBitmap bmBitmap(":/root/crosshair_cursor_bitmap.png");
  QBitmap bmMask(":/root/crosshair_cursor_mask.png");
  m_DrawingCrosshairCursor = new QCursor(bmBitmap, bmMask, 7, 7);

  // New stuff - set up the target panel for QPainter
  QHBoxLayout *lo = new QHBoxLayout(ui->sliceViewNew);
  lo->setContentsMargins(0,0,0,0);
  lo->setSpacing(0);
  ui->sliceViewNew->setLayout(lo);
  auto *canvas = new QtFrameBufferOpenGLWidget(ui->sliceViewNew); //  FrameBufferOpenGLWidget(ui->sliceViewNew);
  canvas->grabGesture(Qt::PinchGesture);
  lo->addWidget(canvas);

  // Create a renderer
  m_Renderer = GenericSliceRenderer::New();
  canvas->SetRenderer(m_Renderer.GetPointer());
  m_RendererCanvas = canvas;
  m_RendererCanvas->setObjectName("sliceViewCanvas");

  // Create a viewport reporter and associate it with the main stack
  // TODO: eventually this should just be m_RendererCanvas
  m_ViewportReporter = QtViewportReporter::New();
  m_ViewportReporter->SetClientWidget(m_RendererCanvas);

  // Create the interaction modes
  m_DelegateChain = new SliceViewDelegateChain(ui->sliceViewNew, m_RendererCanvas, m_Renderer);

  // Configure wheel event target on crosshairs mode
  m_DelegateChain->m_CrosshairsMode->SetWheelEventTargetWidget(ui->inSlicePosition);

  // Connect the context menu signal from polygon mode to this widget
  connect(m_DelegateChain->m_PolygonMode, SIGNAL(contextMenuRequested()), SLOT(onContextMenu()));

  // Configure the context tool button
  m_ContextToolButton = new QToolButton(m_RendererCanvas);
  m_ContextToolButton->setIcon(QIcon(":/root/context_gray_10.png"));
  m_ContextToolButton->setVisible(false);
  m_ContextToolButton->setAutoRaise(true);
  m_ContextToolButton->setIconSize(QSize(10,10));
  m_ContextToolButton->setMinimumSize(QSize(16,16));
  m_ContextToolButton->setMaximumSize(QSize(16,16));
  m_ContextToolButton->setPopupMode(QToolButton::InstantPopup);
  m_ContextToolButton->setStyleSheet("QToolButton::menu-indicator { image: none; }");

  // And also connect toolbar buttons to the corresponding slots in polygon mode
  auto *polygon_mode = m_DelegateChain->m_PolygonMode;
  connect(ui->actionAccept, &QAction::triggered, polygon_mode, &PolygonDrawingInteractionMode::onAcceptPolygon);
  connect(ui->actionPaste, &QAction::triggered, polygon_mode, &PolygonDrawingInteractionMode::onPastePolygon);
  connect(ui->actionClearDrawing, &QAction::triggered, polygon_mode, &PolygonDrawingInteractionMode::onCancelDrawing);
  connect(ui->actionComplete, &QAction::triggered, polygon_mode, &PolygonDrawingInteractionMode::onCloseLoopAndEdit);
  connect(ui->actionDeleteSelected, &QAction::triggered, polygon_mode, &PolygonDrawingInteractionMode::onDeleteSelected);
  connect(ui->actionClearPolygon, &QAction::triggered, polygon_mode, &PolygonDrawingInteractionMode::onClearPolygon);
  connect(ui->actionSplitSelected, &QAction::triggered, polygon_mode, &PolygonDrawingInteractionMode::onSplitSelected);
  connect(ui->actionUndo, &QAction::triggered, polygon_mode, &PolygonDrawingInteractionMode::onUndoLastPoint);

  // Create the popup menus for the polygon mode
  QString menuStyle = "font-size: 12px;";
  m_MenuPolyInactive = new QMenu(polygon_mode);
  m_MenuPolyInactive->setStyleSheet(menuStyle);
  m_MenuPolyInactive->addAction(ui->actionPaste);

  m_MenuPolyDrawing = new QMenu(polygon_mode);
  m_MenuPolyDrawing->setStyleSheet(menuStyle);
  m_MenuPolyDrawing->addAction(ui->actionComplete);
  m_MenuPolyDrawing->addAction(ui->actionCompleteAndAccept);
  m_MenuPolyDrawing->addAction(ui->actionUndo);
  m_MenuPolyDrawing->addAction(ui->actionClearDrawing);

  m_MenuPolyEditing = new QMenu(polygon_mode);
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

  ui->btnAnnotationAcceptLine->setDefaultAction(ui->actionAnnotationAcceptLine);
  ui->btnAnnotationClearLine->setDefaultAction(ui->actionAnnotationClearLine);
  ui->btnAnnotationSelectAll->setDefaultAction(ui->actionAnnotationSelectAll);
  ui->btnAnnotationEdit->setDefaultAction(ui->actionAnnotationEdit);
  ui->btnAnnotationDeleteSelected->setDefaultAction(ui->actionAnnotationDelete);
  ui->btnAnnotationNext->setDefaultAction(ui->actionAnnotationNext);
  ui->btnAnnotationPrevious->setDefaultAction(ui->actionAnnotationPrevious);

  this->addAction(ui->actionZoom_In);
  this->addAction(ui->actionZoom_Out);
}

SliceViewPanel::~SliceViewPanel()
{
  delete ui;
  delete m_DrawingCrosshairCursor;
  delete m_DelegateChain;
}

void SliceViewPanel::Initialize(GlobalUIModel *model, unsigned int index)
{
  // Store the model
  this->m_GlobalUI = model;
  this->m_Index = index;

  // Get the slice model
  m_SliceModel = m_GlobalUI->GetSliceModel(index);

  // Assign the viewport reporter to the model
  m_SliceModel->SetSizeReporter(m_ViewportReporter.GetPointer());

  // Initialize the interaction modes
  m_DelegateChain->SetModel(model, index);

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

  // Connect renderer to model
  m_Renderer->SetModel(m_SliceModel);
  connectITK(m_Renderer, IRISEvent());

  // Correct all the subrenderers to their models
}

void
SliceViewPanel::onModelUpdate(const EventBucket &eb)
{
  if (eb.HasEvent(ToolbarModeChangeEvent()) || eb.HasEvent(StateMachineChangeEvent()))
  {
    OnToolbarModeChange();
  }
  if (eb.HasEvent(DisplayLayoutModel::ViewPanelLayoutChangeEvent()) ||
      eb.HasEvent(DisplayLayoutModel::LayerLayoutChangeEvent()))
  {
    UpdateExpandViewButton();
  }

  // this is causing crash on Linux
  m_RendererCanvas->update();
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

void
SliceViewPanel::SaveScreenshot(const std::string &filename)
{
  auto *w = dynamic_cast<QtFrameBufferOpenGLWidget *>(m_RendererCanvas);
  if (w)
  {
    w->setScreenshotRequest(filename);
    w->update();
  }
}

void SliceViewPanel::OnToolbarModeChange()
{
  auto tb_mode = m_GlobalUI->GetGlobalState()->GetToolbarMode();

  // Configure the interaction mode widgets' event chain and the renderer
  // order
  m_DelegateChain->ConfigureEventChain(tb_mode);

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
    ui->btnExpand->setToolTip(tr("Restore the four-panel display configuration"));
    }
  else
    {
    ui->btnExpand->setToolTip(tr("Expand this view to occupy the entire window"));
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
    int y = (m_RendererCanvas->height() - 1) - (vp->pos[1] + vp->size[1]) / vppr;
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

