#include "InterpolateLabelsDialog.h"
#include "ui_InterpolateLabelsDialog.h"
#include "InterpolateLabelModel.h"

#include "QtComboBoxCoupling.h"
#include "QtRadioButtonCoupling.h"
#include "QtDoubleSpinBoxCoupling.h"
#include "QtCheckBoxCoupling.h"

InterpolateLabelsDialog::InterpolateLabelsDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::InterpolateLabelsDialog)
{
  ui->setupUi(this);
}

InterpolateLabelsDialog::~InterpolateLabelsDialog()
{
  delete ui;
}

void InterpolateLabelsDialog::SetModel(InterpolateLabelModel *model)
{
  m_Model = model;

  makeCoupling(ui->inActiveLabel, m_Model->GetDrawingLabelModel());
  makeCoupling(ui->inLabelToInterpolate, m_Model->GetInterpolateLabelModel());
  makeCoupling(ui->inDrawOver, m_Model->GetDrawOverFilterModel());

  makeRadioGroupCoupling(ui->btnInterpolateAll, ui->btnInterpolateOne, m_Model->GetInterpolateAllModel());

  makeCoupling(ui->inDistanceSmoothing, m_Model->GetSmoothingModel());

  makeCoupling(ui->chkRetain, m_Model->GetRetainScaffoldModel());
  makeCoupling(ui->chkUseLevelSet, m_Model->GetUseLevelSetModel());
  makeCoupling(ui->inLevelSetCurv, m_Model->GetLevelSetCurvatureModel());
}

void InterpolateLabelsDialog::on_btnInterpolate_clicked()
{
  m_Model->Interpolate();
}

void InterpolateLabelsDialog::on_btnClose_clicked()
{
  this->close();
}
