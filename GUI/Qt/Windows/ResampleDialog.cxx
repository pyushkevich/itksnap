#include "ResampleDialog.h"
#include "ui_ResampleDialog.h"
#include "SnakeROIResampleModel.h"
#include "QtDoubleSpinBoxCoupling.h"
#include "QtWidgetArrayCoupling.h"
#include "QtComboBoxCoupling.h"

ResampleDialog::ResampleDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::ResampleDialog)
{
  ui->setupUi(this);
}

ResampleDialog::~ResampleDialog()
{
  delete ui;
}

void ResampleDialog::SetModel(SnakeROIResampleModel *model)
{
  m_Model = model;

  makeArrayCoupling(ui->outInputX, ui->outInputY, ui->outInputZ,
                    model->GetInputSpacingModel());

  makeArrayCoupling(ui->inOutVoxelX, ui->inOutVoxelY, ui->inOutVoxelZ,
                    model->GetOutputSpacingModel());

  makeCoupling(ui->inActionX, model->GetNthScaleFactorModel(0));
  makeCoupling(ui->inActionY, model->GetNthScaleFactorModel(1));
  makeCoupling(ui->inActionZ, model->GetNthScaleFactorModel(2));
}
