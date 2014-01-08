#include "AboutDialog.h"
#include "ui_AboutDialog.h"
#include "QFile"
#include "SNAPCommon.h"

AboutDialog::AboutDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::AboutDialog)
{
  ui->setupUi(this);

  // Load the credits file
  QFile fCredits(":root/credits.html");
  if(fCredits.open(QFile::ReadOnly))
    ui->outCredits->setHtml(QString(fCredits.readAll()));

  // Load the contents into the browser
  QFile fLicense(":/root/license.txt");
  if(fLicense.open(QFile::ReadOnly))
    ui->outLicense->setPlainText(QString(fLicense.readAll()));

  // Load the build information
  ui->outBuild->setPlainText(QString::fromUtf8(SNAPBuildInfo));
}

AboutDialog::~AboutDialog()
{
  delete ui;
}
