#include "AboutDialog.h"
#include "ui_AboutDialog.h"
#include "QFile"


AboutDialog::AboutDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::AboutDialog)
{
  ui->setupUi(this);

  // Load the contents into the browser
  QFile fLicense(":/root/license.txt");
  if(fLicense.open(QFile::ReadOnly))
    {
    ui->outLicense->setPlainText(QString(fLicense.readAll()));
    }
}

AboutDialog::~AboutDialog()
{
  delete ui;
}
