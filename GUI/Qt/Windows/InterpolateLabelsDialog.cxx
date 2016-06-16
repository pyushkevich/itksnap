#include "InterpolateLabelsDialog.h"
#include "ui_InterpolateLabelsDialog.h"
#include "InterpolateLabelModel.h"

#include "QtComboBoxCoupling.h"
#include "QtRadioButtonCoupling.h"
#include "QtDoubleSpinBoxCoupling.h"
#include "QtCheckBoxCoupling.h"

Q_DECLARE_METATYPE(InterpolateLabelModel::InterpolationType)

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
//    std::map<GlobalDisplaySettings::UISliceLayout, QAbstractButton *> btnmap;
//    btnmap[GlobalDisplaySettings::LAYOUT_ACS] = ui->btnACS;
//    btnmap[GlobalDisplaySettings::LAYOUT_ASC] = ui->btnASC;
//    btnmap[GlobalDisplaySettings::LAYOUT_CAS] = ui->btnCAS;
//    btnmap[GlobalDisplaySettings::LAYOUT_CSA] = ui->btnCSA;
//    btnmap[GlobalDisplaySettings::LAYOUT_SAC] = ui->btnSAC;
//    btnmap[GlobalDisplaySettings::LAYOUT_SCA] = ui->btnSCA;
//    makeRadioGroupCoupling(ui->grpLayoutRadio, btnmap, gds->GetSliceLayoutModel());
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

  ui->interpolationMethod->clear();
  ui->interpolationMethod->addItem("Default", QVariant::fromValue(InterpolateLabelModel::DEFAULT));
  ui->interpolationMethod->addItem("Level set", QVariant::fromValue(InterpolateLabelModel::LEVEL_SET));
  ui->interpolationMethod->addItem("Morphology", QVariant::fromValue(InterpolateLabelModel::MORPHOLOGY));

  makeCoupling(ui->interpolationMethod, m_Model->GetInterpolationMethodModel());
//  connect(ui->interpolationMethod, SIGNAL(activated(int)), ui->stackedWidget, SLOT(setCurrentIndex(int)));
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
