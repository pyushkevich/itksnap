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
#include "LabelEditorModel.h"
#include "QMenu"
#include "LabelEditorDialog.h"

LabelInspector::LabelInspector(QWidget *parent) :
  SNAPComponent(parent),
  ui(new Ui::LabelInspector)
{
  ui->setupUi(this);
  ui->inForeLabel->setIconSize(QSize(16,16));
  ui->inBackLabel->setIconSize(QSize(16,16));

  // Find the label editor
  LabelEditorDialog *led =
    dynamic_cast<LabelEditorDialog *>(FindUpstreamDialog(this, "LabelEditorDialog"));
  QObject::connect(ui->actionNew_label, &QAction::triggered, led, &LabelEditorDialog::createNewLabel);
  QObject::connect(ui->actionDuplicate_Label, &QAction::triggered, led, &LabelEditorDialog::duplicateLabel);

  // Connect to the action in the menubar
  QMenu *contextMenu = new QMenu(ui->btnLabelContext);
  contextMenu->addAction(ui->actionEdit_Label);
  contextMenu->addAction(ui->actionNew_label);
  contextMenu->addAction(ui->actionDuplicate_Label);
  contextMenu->addSeparator();
  contextMenu->addAction(ui->actionLocate_Center_of_Mass);
  ui->btnLabelContext->setMenu(contextMenu);
  ui->btnLabelContext->setPopupMode(QToolButton::InstantPopup);
  ui->btnLabelContext->setStyleSheet("QToolButton::menu-indicator { image: none; }");

  ui->bntLabelSwap->setDefaultAction(FindUpstreamAction(this, "actionSwitch_Foreground_Background_Labels"));
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


void
LabelInspector::on_actionEdit_Label_triggered()
{
  // Select the current label
  m_Model->GetLabelEditorModel()->GetCurrentLabelModel()->SetValue(
    m_Model->GetGlobalState()->GetDrawingColorLabel());

  // Open the label editor
  FindUpstreamAction(this, "actionLabel_Editor")->trigger();
}

void
LabelInspector::on_actionLocate_Center_of_Mass_triggered()
{
  auto label = m_Model->GetGlobalState()->GetDrawingColorLabel();
  if (!m_Model->GetDriver()->LocateLabelCenterOfMass(label))
    ReportNonLethalException(this,
                             IRISException(),
                             tr("Failed to Locate Center of Mass"),
                             tr("There are no voxels with label %1 in the segmentation").arg(label));
}

void
LabelInspector::on_actionNew_label_triggered()
{

}
