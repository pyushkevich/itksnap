#include "SliceViewPanel.h"
#include "ui_SliceViewPanel.h"

#include "GlobalUIModel.h"
#include "SNAPEvents.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "itkCommand.h"
#include "CrosshairsRenderer.h"
#include "PolygonDrawingRenderer.h"
#include "SliceWindowCoordinator.h"
#include "PolygonDrawingModel.h"
#include "QtWidgetActivator.h"

#include <QStackedLayout>
#include <QMenu>

SliceViewPanel::SliceViewPanel(QWidget *parent) :
    SNAPComponent(parent),
    ui(new Ui::SliceViewPanel)
{
  ui->setupUi(this);

  QString menuStyle = "font-size: 12pt;";

  // Create the popup menus for the polygon mode
  m_MenuPolyInactive = new QMenu(ui->imPolygon);
  m_MenuPolyInactive->setStyleSheet(menuStyle);
  m_MenuPolyInactive->addAction(ui->actionPaste);

  m_MenuPolyDrawing = new QMenu(ui->imPolygon);
  m_MenuPolyDrawing->setStyleSheet(menuStyle);
  m_MenuPolyDrawing->addAction(ui->actionComplete);
  m_MenuPolyDrawing->addAction(ui->actionCompleteAndAccept);
  m_MenuPolyDrawing->addAction(ui->actionUndo);
  m_MenuPolyDrawing->addAction(ui->actionClearDrawing);

  m_MenuPolyEditing = new QMenu(ui->imPolygon);
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

  // Connect the context menu signal from polygon mode to this widget
  connect(ui->imPolygon, SIGNAL(contextMenuRequested()), SLOT(onContextMenu()));

  // Arrange the interaction modes into a tree structure. The first child of
  // every interaction mode is an empty QWidget. The tree is used to allow
  // events to fall through from one interaction mode to another
  QStackedLayout *loMain = new QStackedLayout();
  loMain->addWidget(ui->imCrosshairs);
  loMain->addWidget(ui->imZoomPan);
  loMain->addWidget(ui->imThumbnail);
  delete ui->sliceView->layout();
  ui->sliceView->setLayout(loMain);

  QStackedLayout *loCrosshair = new QStackedLayout();
  loCrosshair->addWidget(new QWidget());
  loCrosshair->addWidget(ui->imPolygon);
  delete ui->imCrosshairs->layout();
  ui->imCrosshairs->setLayout(loCrosshair);

  // Also lay out the pages
  QStackedLayout *loPages = new QStackedLayout();
  loPages->addWidget(ui->pageDefault);
  loPages->addWidget(ui->pagePolygonDraw);
  loPages->addWidget(ui->pagePolygonEdit);
  loPages->addWidget(ui->pagePolygonInactive);
  delete ui->toolbar->layout();
  ui->toolbar->setLayout(loPages);

  // Let the thumbnail code filter all the events from other modes
  ui->imCrosshairs->installEventFilter(ui->imThumbnail);
  ui->imZoomPan->installEventFilter(ui->imThumbnail);
  ui->imPolygon->installEventFilter(ui->imThumbnail);

  // Events not picked up by the polygon mode should filter through to
  // the crosshairs mode

  // Send wheel events from Crosshairs mode to the slider
  ui->imCrosshairs->SetWheelEventTargetWidget(ui->inSlicePosition);
}

SliceViewPanel::~SliceViewPanel()
{
  delete ui;
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

  // Initialize the slice view
  ui->sliceView->SetModel(m_GlobalUI->GetSliceModel(index));

  // Initialize the interaction modes
  ui->imCrosshairs->SetModel(m_GlobalUI->GetCursorNavigationModel(index));

  ui->imZoomPan->SetModel(m_GlobalUI->GetCursorNavigationModel(index));
  ui->imZoomPan->SetMouseButtonBehaviorToZoomPanMode();

  ui->imThumbnail->SetModel(m_GlobalUI->GetCursorNavigationModel(index));

  ui->imPolygon->SetModel(m_GlobalUI->GetPolygonDrawingModel(index));

  // Attach the overlays to the master renderer. Why are we doing it here?
  GenericSliceRenderer::RendererDelegateList &overlays =
      ui->sliceView->GetRendererOverlays();
  overlays.push_back(ui->imCrosshairs->GetRenderer());
  overlays.push_back(ui->imPolygon->GetRenderer());

  // Add listener for changes to the model
  connectITK(m_GlobalUI->GetSliceModel(index), ModelUpdateEvent());
  connectITK(m_GlobalUI, CursorUpdateEvent());

  // Listen to toolbar change events
  connectITK(m_GlobalUI, ToolbarModeChangeEvent());

  // Listen to polygon state change events
  connectITK(m_GlobalUI->GetPolygonDrawingModel(index),
             StateMachineChangeEvent());

  // Activation
  activateOnFlag(this, m_GlobalUI, UIF_BASEIMG_LOADED);

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


  /*
  AddListener(m_GlobalUI->GetSliceModel(index), GenericSliceModel::ModelUpdateEvent(),
              this, &SliceViewPanel::OnModelUpdate); */

  // Listen to changes to the crosshairs (for the slider)
  // AddListener(m_GlobalUI->GetDriver(), CursorUpdateEvent(),
  //            this, &SliceViewPanel::OnCursorUpdate);

  // Listen to changes to the model (to set slider min/max)
  // AddListener(m_GlobalUI->GetSliceModel(index), SliceModelImageDimensionsChangeEvent(),
  //            this, &SliceViewPanel::OnImageDimensionsUpdate);

  // Listen to changes to the toolbar mode (to change the interactor)
  // AddListener(m_GlobalUI, ToolbarModeChangeEvent(),
  //            this, &SliceViewPanel::OnToolbarModeChange);
}

void SliceViewPanel::onModelUpdate(const EventBucket &eb)
{
  if(eb.HasEvent(ModelUpdateEvent()) || eb.HasEvent(CursorUpdateEvent()))
    {
    UpdateSlicePositionWidgets();
    }
  if(eb.HasEvent(ToolbarModeChangeEvent()) ||
     eb.HasEvent(StateMachineChangeEvent()))
    {
    OnToolbarModeChange();
    }
  ui->sliceView->update();
}

void SliceViewPanel::UpdateSlicePositionWidgets()
{
  // Be sure to update the model
  this->GetSliceView()->GetModel()->Update();

  // Get the current slice index
  int pos = (int) GetSliceView()->GetModel()->GetSliceIndex();

  // Get the current slice index
  int dim = (int) GetSliceView()->GetModel()->GetNumberOfSlices();

  // Update the slider
  ui->inSlicePosition->setValue(pos);
  ui->inSlicePosition->setMaximum(dim - 1);
  ui->inSlicePosition->setSingleStep(1);
  ui->inSlicePosition->setPageStep(5);

  // Update the text display
  ui->lblSliceInfo->setText(QString("%1 of %2").arg(pos+1).arg(dim));
}

void SliceViewPanel::OnCursorUpdate()
{
  UpdateSlicePositionWidgets();

  // Request a redraw of the GUI
  ui->sliceView->update();
}

void SliceViewPanel::OnImageDimensionsUpdate()
{
  // Get the current slice index
  UpdateSlicePositionWidgets();
}

void SliceViewPanel::on_inSlicePosition_valueChanged(int value)
{
  // Update the cursor position in the model
  if(value != (int) GetSliceView()->GetModel()->GetSliceIndex())
    this->GetSliceView()->GetModel()->UpdateSliceIndex(value);
}

void SliceViewPanel::SetActiveMode(QWidget *mode, bool clearChildren)
{
  // If the widget does not have a stacked layout, do nothing, we've reached
  // the end of the recursion
  QStackedLayout *loParent =
      dynamic_cast<QStackedLayout *>(mode->parentWidget()->layout());

  if(loParent)
    {
    // Set the mode as the current widget
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
  switch((ToolbarModeType)m_GlobalUI->GetToolbarMode())
    {
  case POLYGON_DRAWING_MODE:
    SetActiveMode(ui->imPolygon);
    break;
  case PAINTBRUSH_MODE:
    break;
  case ANNOTATION_MODE:
    break;
  case ROI_MODE:
    break;
  case CROSSHAIRS_MODE:
    SetActiveMode(ui->imCrosshairs);
    break;
  case NAVIGATION_MODE:
    SetActiveMode(ui->imZoomPan);
    break;
    }

  // Need to do change to the appropriate page
  QStackedLayout *loPages =
      static_cast<QStackedLayout *>(ui->toolbar->layout());
  if(m_GlobalUI->GetToolbarMode() == POLYGON_DRAWING_MODE)
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
  else
    loPages->setCurrentWidget(ui->pageDefault);
}

void SliceViewPanel::on_btnZoomToFit_clicked()
{
  m_GlobalUI->GetSliceCoordinator()->ResetViewToFitInOneWindow(m_Index);

}

void SliceViewPanel::onContextMenu()
{
  if(m_GlobalUI->GetToolbarMode() == POLYGON_DRAWING_MODE)
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
