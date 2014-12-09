#include "LabelInspector.h"
#include "ui_LabelInspector.h"
#include <SNAPQtCommon.h>
#include "GlobalUIModel.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "QtComboBoxCoupling.h"
#include "QtCheckBoxCoupling.h"
#include "QtSliderCoupling.h"
#include "QtSpinBoxCoupling.h"

LabelInspector::LabelInspector(QWidget *parent) :
  SNAPComponent(parent),
  ui(new Ui::LabelInspector)
{
  ui->setupUi(this);
  ui->inForeLabel->setIconSize(QSize(16,16));
  ui->inBackLabel->setIconSize(QSize(16,16));

  // Connect to the action in the menubar
  // ui->btnEdit->setAction("actionLabel_Editor");
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

  // Attach to quick list
  // ui->quickList->SetModel(model);

  // Use couplings where we can
  makeCoupling(ui->inOpacity, m_Model->GetSegmentationOpacityModel());
  makeCoupling(ui->inOpacityValue, m_Model->GetSegmentationOpacityModel());
  // makeCoupling(ui->chkVisible, m_Model->GetSegmentationVisibilityModel());

  // Couple the color label combo box. The actual logic for how the labels are
  // mapped to color labels is handled in QtComboBoxCoupling.h
  makeCoupling(ui->inForeLabel,
               m_Model->GetGlobalState()->GetDrawingColorLabelModel());

  // Couple the draw over label combo box.
  makeCoupling(ui->inBackLabel,
               m_Model->GetGlobalState()->GetDrawOverFilterModel());

}


