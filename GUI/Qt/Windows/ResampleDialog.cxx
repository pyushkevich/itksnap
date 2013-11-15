#include "ResampleDialog.h"
#include "ui_ResampleDialog.h"
#include "SnakeROIResampleModel.h"
#include "QtDoubleSpinBoxCoupling.h"
#include "QtWidgetArrayCoupling.h"
#include "QtSpinBoxCoupling.h"
#include "QtCheckBoxCoupling.h"
#include "QtComboBoxCoupling.h"
#include <QMenu>

Q_DECLARE_METATYPE(SNAPSegmentationROISettings::InterpolationMethod)

ResampleDialog::ResampleDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::ResampleDialog)
{
  ui->setupUi(this);

  QMenu *menu = new QMenu(this);
  menu->addAction(ui->actionSuper2);
  menu->addAction(ui->actionSub2);
  menu->addSeparator();
  menu->addAction(ui->actionSuperIso);
  menu->addAction(ui->actionSubIso);

  ui->btnPreset->setMenu(menu);
}

ResampleDialog::~ResampleDialog()
{
  delete ui;
}

void ResampleDialog::SetModel(SnakeROIResampleModel *model)
{
  m_Model = model;

  makeCoupling(ui->outInputXvox, model->GetInputSpacingModel(0));
  makeCoupling(ui->outInputYvox, model->GetInputSpacingModel(1));
  makeCoupling(ui->outInputZvox, model->GetInputSpacingModel(2));

  makeCoupling(ui->outInputXsize, model->GetInputDimensionsModel(0));
  makeCoupling(ui->outInputYsize, model->GetInputDimensionsModel(1));
  makeCoupling(ui->outInputZsize, model->GetInputDimensionsModel(2));

  makeCoupling(ui->inOutVoxelXvox, model->GetOutputSpacingModel(0));
  makeCoupling(ui->inOutVoxelYvox, model->GetOutputSpacingModel(1));
  makeCoupling(ui->inOutVoxelZvox, model->GetOutputSpacingModel(2));

  makeCoupling(ui->inOutVoxelXsize, model->GetOutputDimensionsModel(0));
  makeCoupling(ui->inOutVoxelYsize, model->GetOutputDimensionsModel(1));
  makeCoupling(ui->inOutVoxelZsize, model->GetOutputDimensionsModel(2));

  makeCoupling(ui->inInterpolationMode, model->GetInterpolationModeModel());

  makeCoupling(ui->chkAspect, model->GetFixedAspectRatioModel());

}

void ResampleDialog::on_buttonBox_clicked(QAbstractButton *button)
{
  switch(ui->buttonBox->standardButton(button))
    {
    case QDialogButtonBox::Reset:
      m_Model->Reset();
      return;
    case QDialogButtonBox::Ok:
      m_Model->Accept();
      this->accept();
      return;
    case QDialogButtonBox::Cancel:
      this->reject();
      return;
    default:
      return;
    }
}

void ResampleDialog::on_actionSuper2_triggered()
{
  m_Model->ApplyPreset(SnakeROIResampleModel::SUPER_2);
}

void ResampleDialog::on_actionSub2_triggered()
{
  m_Model->ApplyPreset(SnakeROIResampleModel::SUB_2);
}

void ResampleDialog::on_actionSuperIso_triggered()
{
  m_Model->ApplyPreset(SnakeROIResampleModel::SUPER_ISO);
}

void ResampleDialog::on_actionSubIso_triggered()
{
  m_Model->ApplyPreset(SnakeROIResampleModel::SUB_ISO);
}
