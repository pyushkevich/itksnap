#include "LabelInspector.h"
#include "ui_LabelInspector.h"
#include <SNAPQtCommon.h>
#include "GlobalUIModel.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "QtSliderCoupling.h"
#include "QtComboBoxCoupling.h"
#include "QtCheckBoxCoupling.h"

LabelInspector::LabelInspector(QWidget *parent) :
  SNAPComponent(parent),
  ui(new Ui::LabelInspector)
{
  ui->setupUi(this);
  ui->inForeLabel->setIconSize(QSize(16,16));
  ui->inBackLabel->setIconSize(QSize(16,16));
}

LabelInspector::~LabelInspector()
{
  delete ui;
}

void LabelInspector
::SetModel(GlobalUIModel *model)
{
  // Get the model
  m_Model = model;

  // Use couplings where we can
  makeCoupling(ui->inOpacity,
               m_Model->GetDriver()->GetGlobalState()->GetSegmentationAlphaModel());

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



void LabelInspector::on_btnEdit_clicked()
{
  TriggerUpstreamAction(this, "actionLabel_Editor");
}


