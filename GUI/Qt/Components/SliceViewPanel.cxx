#include "SliceViewPanel.h"
#include "ui_SliceViewPanel.h"

#include "GlobalUIModel.h"
#include "SNAPEvents.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "itkCommand.h"
#include "CrosshairsRenderer.h"
#include "SliceWindowCoordinator.h"

#include <QStackedLayout>

SliceViewPanel::SliceViewPanel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SliceViewPanel)
{
  ui->setupUi(this);

  // Set the layouts
  QStackedLayout *layout = new QStackedLayout();
  layout->addWidget(ui->imCrosshairs);
  layout->addWidget(ui->imZoomPan);
  layout->addWidget(ui->imThumbnail);
  ui->sliceView->setLayout(layout);

  // Let the thumbnail code filter all the events from other modes
  ui->imCrosshairs->installEventFilter(ui->imThumbnail);
  ui->imZoomPan->installEventFilter(ui->imThumbnail);

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

  // Attach the overlays to the master renderer. Why are we doing it here?
  GenericSliceRenderer::RendererDelegateList &overlays =
      ui->sliceView->GetRenderer().GetOverlays();
  overlays.push_back(&ui->imCrosshairs->GetRenderer());

  // Listen to changes to the crosshairs (for the slider)
  AddListener(m_GlobalUI->GetDriver(), CursorUpdateEvent(),
              this, &SliceViewPanel::OnCursorUpdate);

  // Listen to changes to the model (to set slider min/max)
  AddListener(m_GlobalUI->GetSliceModel(index), SliceModelImageDimensionsChangeEvent(),
              this, &SliceViewPanel::OnImageDimensionsUpdate);

  // Listen to changes to the toolbar mode (to change the interactor)
  AddListener(m_GlobalUI, ToolbarModeChangeEvent(),
              this, &SliceViewPanel::OnToolbarModeChange);
}

void SliceViewPanel::UpdateSlicePositionWidgets()
{
  // Get the current slice index
  int pos = (int) GetSliceView()->GetModel()->GetSliceIndex();

  // Get the current slice index
  int dim = (int) GetSliceView()->GetModel()->GetNumberOfSlices();

  // Update the slider
  ui->inSlicePosition->setValue(pos);
  ui->inSlicePosition->setMaximum(dim - 1);
  ui->inSlicePosition->setSingleStep(1);
  ui->inSlicePosition->setPageStep(1);

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

void SliceViewPanel::OnToolbarModeChange()
{
  QStackedLayout *layout =
      static_cast<QStackedLayout *>(ui->sliceView->layout());

  switch(m_GlobalUI->GetToolbarMode())
    {
    case CROSSHAIRS_MODE:
      layout->setCurrentWidget(ui->imCrosshairs);
      break;
    case NAVIGATION_MODE:
      layout->setCurrentWidget(ui->imZoomPan);
      break;
    }
}

void SliceViewPanel::on_btnZoomToFit_clicked()
{
  m_GlobalUI->GetSliceCoordinator()->ResetViewToFitInOneWindow(m_Index);

}
