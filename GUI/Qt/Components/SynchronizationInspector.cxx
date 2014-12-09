#include "SynchronizationInspector.h"
#include "ui_SynchronizationInspector.h"
#include "SynchronizationModel.h"
#include "QtSpinBoxCoupling.h"
#include "QtCheckBoxCoupling.h"

SynchronizationInspector::SynchronizationInspector(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::SynchronizationInspector)
{
  ui->setupUi(this);

  // TODO: add the code to support multiple channels
  ui->panelChannel->setVisible(false);
}

SynchronizationInspector::~SynchronizationInspector()
{
  delete ui;
}

void SynchronizationInspector::SetModel(SynchronizationModel *model)
{
  m_Model = model;

  // Do couplings
  makeCoupling(ui->chkSync, model->GetSyncEnabledModel());
  makeCoupling(ui->chkCursor, model->GetSyncCursorModel());
  makeCoupling(ui->chkZoom, model->GetSyncZoomModel());
  makeCoupling(ui->chkPan, model->GetSyncPanModel());
  makeCoupling(ui->chkCamera, model->GetSyncCameraModel());

  // The checkboxes should be deactivated when the sync model is off
  makeBooleanNamedPropertyCoupling(ui->panelProperties, "enabled",
                                   model->GetSyncEnabledModel());
}
