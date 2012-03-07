#include "ColorMapInspector.h"
#include "ui_ColorMapInspector.h"
#include "ColorMapModel.h"
#include "QtReporterDelegates.h"
#include "QtWidgetCoupling.h"
#include "QtWidgetActivator.h"

ColorMapInspector::ColorMapInspector(QWidget *parent) :
  SNAPComponent(parent),
  ui(new Ui::ColorMapInspector)
{
  ui->setupUi(this);

  // Create the viewport reporter
  m_ColorMapBoxViewportReporter = new QtViewportReporter(ui->boxColorMap);
}

ColorMapInspector::~ColorMapInspector()
{
  delete ui;
}

void ColorMapInspector::SetModel(ColorMapModel *model)
{
  // Set the model
  m_Model = model;
  ui->boxColorMap->SetModel(model);

  // Connect the viewport reporter
  model->SetViewportReporter(m_ColorMapBoxViewportReporter);

  // Listen to model update events
  connectITK(m_Model, ModelUpdateEvent());

  // Connect widgets to the corresponding sub-models
  makeCoupling(ui->inControlX, m_Model->GetMovingControlPositionModel());
  makeCoupling(ui->inControlOpacity, m_Model->GetMovingControlOpacityModel());
  // makeCoupling(ui->btnCont, m_Model->GetMovingControlIsContinuousModel());
  // makeCoupling(ui->btnLeft, m_Model->GetMovingControlIsLeftModel());

  makeRadioGroupCoupling(ui->grpRadioCont,
                         m_Model->GetMovingControlContinuityModel());

  makeRadioGroupCoupling(ui->grpRadioSide,
                         m_Model->GetMovingControlSideModel());

  // Set up activations
  activateOnFlag(ui->inControlX, m_Model,
                 ColorMapModel::UIF_CONTROL_SELECTED_IS_NOT_ENDPOINT);
  activateOnFlag(ui->inControlOpacity, m_Model,
                 ColorMapModel::UIF_CONTROL_SELECTED);
  activateOnFlag(ui->btnCont, m_Model,
                 ColorMapModel::UIF_CONTROL_SELECTED_IS_NOT_ENDPOINT);
  activateOnFlag(ui->btnDiscont, m_Model,
                 ColorMapModel::UIF_CONTROL_SELECTED_IS_NOT_ENDPOINT);
  activateOnFlag(ui->btnLeft, m_Model,
                 ColorMapModel::UIF_CONTROL_SELECTED_IS_DISCONTINUOUS);
  activateOnFlag(ui->btnRight, m_Model,
                 ColorMapModel::UIF_CONTROL_SELECTED_IS_DISCONTINUOUS);
}

void ColorMapInspector::onModelUpdate(const EventBucket &b)
{
  m_Model->Update();
}


