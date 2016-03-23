#include "InterpolateLabelsDialog.h"
#include "ui_InterpolateLabelsDialog.h"

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
