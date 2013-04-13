#include "LabelMiniInspector.h"
#include "ui_LabelMiniInspector.h"
#include <SNAPQtCommon.h>
#include "GlobalUIModel.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "QtComboBoxCoupling.h"
#include "QtCheckBoxCoupling.h"
#include "QtSliderCoupling.h"
#include "QtSpinBoxCoupling.h"
#include "QtAbstractButtonCoupling.h"

LabelMiniInspector::LabelMiniInspector(QWidget *parent) :
  SNAPComponent(parent),
  ui(new Ui::LabelMiniInspector)
{
  ui->setupUi(this);
  ui->inForeLabel->setIconSize(QSize(16,16));
  ui->inBackLabel->setIconSize(QSize(16,16));
}

LabelMiniInspector::~LabelMiniInspector()
{
  delete ui;
}

void LabelMiniInspector
::SetModel(GlobalUIModel *model)
{
  // Get the model
  m_Model = model;

  // Use couplings where we can
  makeCoupling(ui->inOpacity, m_Model->GetSegmentationOpacityModel());
  makeCoupling(ui->inOpacityValue, m_Model->GetSegmentationOpacityModel());
  makeCoupling((QAbstractButton *)ui->toolButton, m_Model->GetSegmentationVisibilityModel());

  // Couple the color label combo box. The actual logic for how the labels are
  // mapped to color labels is handled in QtComboBoxCoupling.h
  makeCoupling(ui->inForeLabel,
               m_Model->GetGlobalState()->GetDrawingColorLabelModel());

  // Couple the draw over label combo box.
  makeCoupling(ui->inBackLabel,
               m_Model->GetGlobalState()->GetDrawOverFilterModel());

  // Couple the inversion checkbox
  makeCoupling(ui->cbInvert,
               m_Model->GetGlobalState()->GetPolygonInvertModel());
}


