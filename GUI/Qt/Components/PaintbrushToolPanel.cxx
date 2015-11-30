#include "PaintbrushToolPanel.h"
#include "ui_PaintbrushToolPanel.h"

#include "PaintbrushSettingsModel.h"
#include "QtRadioButtonCoupling.h"
#include "QtCheckBoxCoupling.h"
#include "QtDoubleSpinBoxCoupling.h"
#include "QtSpinBoxCoupling.h"
#include "QtSliderCoupling.h"

PaintbrushToolPanel::PaintbrushToolPanel(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::PaintbrushToolPanel)
{
  ui->setupUi(this);

  // Adjust the shortcuts for increase/decrease behavior
  ui->actionBrushIncrease->setShortcuts(
        ui->actionBrushIncrease->shortcuts() << QKeySequence('='));

  ui->actionBrushDecrease->setShortcuts(
        ui->actionBrushDecrease->shortcuts() << QKeySequence('_'));

  ui->actionGranularityIncrease->setShortcuts(
        ui->actionGranularityIncrease->shortcuts() << QKeySequence(Qt::META + Qt::Key_Equal) << QKeySequence(Qt::META + Qt::Key_Plus));

  ui->actionGranularityDecrease->setShortcuts(
        ui->actionGranularityDecrease->shortcuts() << QKeySequence(Qt::META + Qt::Key_Underscore) << QKeySequence(Qt::META + Qt::Key_Minus));

  ui->actionSmoothnessIncrease->setShortcuts(
        ui->actionSmoothnessIncrease->shortcuts() << QKeySequence(Qt::ALT + Qt::Key_Equal) << QKeySequence(Qt::ALT + Qt::Key_Plus));

  ui->actionSmoothnessDecrease->setShortcuts(
        ui->actionSmoothnessDecrease->shortcuts() << QKeySequence(Qt::ALT + Qt::Key_Underscore) << QKeySequence(Qt::ALT + Qt::Key_Minus));


  addAction(ui->actionBrushIncrease);
  addAction(ui->actionBrushDecrease);
  addAction(ui->actionGranularityDecrease);
  addAction(ui->actionGranularityIncrease);
  addAction(ui->actionSmoothnessDecrease);
  addAction(ui->actionSmoothnessIncrease);
  addAction(ui->actionBrushStyle);
}

PaintbrushToolPanel::~PaintbrushToolPanel()
{
  delete ui;
}

void PaintbrushToolPanel::SetModel(PaintbrushSettingsModel *model)
{
  m_Model = model;

  // Couple the radio buttons
  std::map<PaintbrushMode, QAbstractButton *> rmap;
  rmap[PAINTBRUSH_RECTANGULAR] = ui->btnSquare;
  rmap[PAINTBRUSH_ROUND] = ui->btnRound;
  rmap[PAINTBRUSH_WATERSHED] = ui->btnWatershed;
  makeRadioGroupCoupling(ui->grpBrushStyle, rmap,
                         m_Model->GetPaintbrushModeModel());

  // Couple the other controls
  makeCoupling(ui->chkVolumetric, model->GetVolumetricBrushModel());
  makeCoupling(ui->chkIsotropic, model->GetIsotropicBrushModel());
  makeCoupling(ui->chkChase, model->GetChaseCursorModel());

  makeCoupling(ui->inBrushSizeSlider, model->GetBrushSizeModel());
  makeCoupling(ui->inBrushSizeSpinbox, model->GetBrushSizeModel());

  // Couple the visibility of the adaptive widget
  makeWidgetVisibilityCoupling(ui->grpAdaptive, model->GetAdaptiveModeModel());

  makeCoupling(ui->inGranularity, model->GetThresholdLevelModel());
  makeCoupling(ui->inSmoothness, model->GetSmoothingIterationsModel());
}

void PaintbrushToolPanel::on_actionBrushStyle_triggered()
{
  if(ui->btnSquare->isChecked())
    ui->btnRound->setChecked(true);
  else if(ui->btnRound->isChecked())
    ui->btnWatershed->setChecked(true);
  else if(ui->btnWatershed->isChecked())
    ui->btnSquare->setChecked(true);
}
