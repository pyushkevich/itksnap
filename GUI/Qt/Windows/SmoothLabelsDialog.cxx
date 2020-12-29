// issue #24: Add label smoothing feature

#include "SmoothLabelsDialog.h"
#include "ui_SmoothLabelsDialog.h"
#include "SmoothLabelsModel.h"

#include "QtComboBoxCoupling.h"
#include "QtRadioButtonCoupling.h"

SmoothLabelsDialog::SmoothLabelsDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::SmoothLabelsDialog)
{
  ui->setupUi(this);
}

SmoothLabelsDialog::~SmoothLabelsDialog()
{
  delete ui;
}

void SmoothLabelsDialog::SetModel(SmoothLabelsModel *model)
{
  m_Model = model;

  // add event subscription here
}

void SmoothLabelsDialog::on_btnApply_clicked()
{
  m_Model->Smooth();
}

void SmoothLabelsDialog::on_btnClose_clicked()
{
  this->close();
}

void SmoothLabelsDialog::showEvent(QShowEvent *e)
{
  // Call parent method
  QDialog::showEvent(e);

  // If the widget is not currently showing, update it's state
  m_Model->UpdateOnShow();
}
