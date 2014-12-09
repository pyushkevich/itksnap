#include "PolygonToolPanel.h"
#include "ui_PolygonToolPanel.h"
#include "GlobalUIModel.h"

#include "QtCheckBoxCoupling.h"
#include "QtSpinBoxCoupling.h"
#include "QtSliderCoupling.h"
#include "QtRadioButtonCoupling.h"

#include "GlobalState.h"
#include "PolygonSettingsModel.h"
#include "QtWidgetActivator.h"

PolygonToolPanel::PolygonToolPanel(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::PolygonToolPanel)
{
  ui->setupUi(this);
}

PolygonToolPanel::~PolygonToolPanel()
{
  delete ui;
}

void PolygonToolPanel::SetModel(GlobalUIModel *model)
{
  m_Model = model;

  // Couple the inversion checkbox
  makeCoupling(ui->chkInvertPolygon,
               m_Model->GetGlobalState()->GetPolygonInvertModel());

  // Couple the freehand drawing mode
  makeCoupling(ui->inSegmentLength,
               m_Model->GetPolygonSettingsModel()->GetFreehandSegmentLengthModel());
  makeCoupling(ui->inSegmentLengthSlider,
               m_Model->GetPolygonSettingsModel()->GetFreehandSegmentLengthModel());

  // Couple the radio buttons
  std::map<bool, QAbstractButton *> radioMap;
  radioMap[false] = ui->btnSmooth;
  radioMap[true] = ui->btnPiecewiseLinear;
  makeRadioGroupCoupling(ui->grpCurveStyle, radioMap,
                         m_Model->GetPolygonSettingsModel()->GetFreehandIsPiecewiseModel());

  // Toggle the appearance of the piecewise block
  makeWidgetVisibilityCoupling(ui->grpSegmentLength,
                               m_Model->GetPolygonSettingsModel()->GetFreehandIsPiecewiseModel());

}
