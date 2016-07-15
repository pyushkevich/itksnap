#include "RegistrationDialog.h"
#include "ui_RegistrationDialog.h"

#include "QtComboBoxCoupling.h"
#include "QtDoubleSpinBoxCoupling.h"
#include "QtWidgetArrayCoupling.h"
#include "RegistrationModel.h"

RegistrationDialog::RegistrationDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::RegistrationDialog)
{
  ui->setupUi(this);
}

RegistrationDialog::~RegistrationDialog()
{
  delete ui;
}

void RegistrationDialog::SetModel(RegistrationModel *model)
{
  m_Model = model;

  makeCoupling(ui->inMovingLayer, m_Model->GetMovingLayerModel());
  makeArrayCoupling(ui->inRotX, ui->inRotY, ui->inRotZ,
                    m_Model->GetEulerAnglesModel());

}
