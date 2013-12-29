#include "AboutDialog.h"
#include "ui_AboutDialog.h"
#include "QFile"
#include "SNAPCommon.h"

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

  // Load the build information
  QString temptext = ui->outBuild->toHtml();
  temptext.replace("%BUILD_DATE%", SNAPBuildDate);
  temptext.replace("%GIT_COMMIT%", SNAPGitSignature);
  ui->outBuild->setHtml(temptext);
}

AboutDialog::~AboutDialog()
{
  delete ui;
}
