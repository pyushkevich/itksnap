#include "DeepLearningInfoDialog.h"
#include "ui_DeepLearningInfoDialog.h"

DeepLearningInfoDialog::DeepLearningInfoDialog(QWidget *parent)
  : QDialog(parent)
  , ui(new Ui::DeepLearningInfoDialog)
{
  ui->setupUi(this);
}

DeepLearningInfoDialog::~DeepLearningInfoDialog() { delete ui; }

bool
DeepLearningInfoDialog::isDoNotShowAgainChecked() const
{
  return ui->chkDoNotShowAgain->isChecked();
}

void
DeepLearningInfoDialog::on_btnYes_clicked()
{
  this->accept();
}

void
DeepLearningInfoDialog::on_btnNo_clicked()
{
  this->reject();
}
