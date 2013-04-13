#include "ResampleDialog.h"
#include "ui_ResampleDialog.h"

ResampleDialog::ResampleDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::ResampleDialog)
{
  ui->setupUi(this);
}

ResampleDialog::~ResampleDialog()
{
  delete ui;
}
