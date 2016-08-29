#include "InterpolateLabelsDialog.h"
#include "ui_InterpolateLabelsDialog.h"
#include "InterpolateLabelModel.h"

#include "QtComboBoxCoupling.h"
#include "QtRadioButtonCoupling.h"
#include "QtDoubleSpinBoxCoupling.h"
#include "QtCheckBoxCoupling.h"

Q_DECLARE_METATYPE(InterpolateLabelModel::InterpolationType)
Q_DECLARE_METATYPE(AnatomicalDirection)

InterpolateLabelsDialog::InterpolateLabelsDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::InterpolateLabelsDialog)
{
  ui->setupUi(this);

  ui->stackedWidget->setCurrentIndex(0);
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

  makeCoupling(ui->chkRetain, m_Model->GetRetainScaffoldModel());

  // Settings for default method
  makeCoupling(ui->inDefaultDistanceSmoothing, m_Model->GetDefaultSmoothingModel());

  // Settings for level set method
  makeCoupling(ui->inLevelSetDistanceSmoothing, m_Model->GetLevelSetSmoothingModel());
  makeCoupling(ui->inLevelSetCurv, m_Model->GetLevelSetCurvatureModel());

  // Settings for morphology method
  makeCoupling(ui->chkMorphologyUseDistance, m_Model->GetMorphologyUseDistanceModel());
  makeCoupling(ui->chkMorphologyUseOptimalAlignment, m_Model->GetMorphologyUseOptimalAlignmentModel());
  makeCoupling(ui->chkMorphologyInterpolateOneAxis, m_Model->GetMorphologyInterpolateOneAxisModel());

  ui->interpolationMethod->clear();
  ui->interpolationMethod->addItem("Morphology", QVariant::fromValue(InterpolateLabelModel::MORPHOLOGY));
  ui->interpolationMethod->addItem("Level set", QVariant::fromValue(InterpolateLabelModel::LEVEL_SET));
  ui->interpolationMethod->addItem("Default", QVariant::fromValue(InterpolateLabelModel::DISTANCE_MAP));
  makeCoupling(ui->interpolationMethod, m_Model->GetInterpolationMethodModel());

  ui->morphologyInterpolationAxis->clear();
  ui->morphologyInterpolationAxis->addItem("Axial",QVariant::fromValue(ANATOMY_AXIAL));
  ui->morphologyInterpolationAxis->addItem("Sagittal",QVariant::fromValue(ANATOMY_SAGITTAL));
  ui->morphologyInterpolationAxis->addItem("Coronal",QVariant::fromValue(ANATOMY_CORONAL));
  makeCoupling(ui->morphologyInterpolationAxis, m_Model->GetMorphologyInterpolationAxisModel());
}

void InterpolateLabelsDialog::on_btnInterpolate_clicked()
{
  m_Model->Interpolate();
}

void InterpolateLabelsDialog::on_btnClose_clicked()
{
  this->close();
}

void InterpolateLabelsDialog::on_interpolationMethod_activated(int index)
{
  ui->stackedWidget->setCurrentIndex(index);
}

void InterpolateLabelsDialog::showEvent(QShowEvent *e)
{
  // Call parent method
  QDialog::showEvent(e);

  // If the widget is not currently showing, update it's state
  m_Model->UpdateOnShow();
}
