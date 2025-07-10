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
  fCredits.close();

  // Load the contents into the browser
  QFile fLicense(":/root/license.txt");
  if(fLicense.open(QFile::ReadOnly))
    ui->outLicense->setPlainText(QString(fLicense.readAll()));
  fLicense.close();

  // Load the contents into the browser
  QFile fAlgs(":/root/html/external_citations.md");
  if(fAlgs.open(QFile::ReadOnly))
    ui->ouAlgorithms->setMarkdown(QString(fAlgs.readAll()));
  fAlgs.close();

  // Load the contents into the browser
  QFile fComponents(":/root/html/components.html");
  if(fComponents.open(QFile::ReadOnly))
    ui->outComponents->setHtml(QString(fComponents.readAll()));
  fComponents.close();

  // Load the build information
  ui->outBuild->setHtml(QString("<pre>%1</pre>").arg(QString::fromUtf8(SNAPBuildInfo)));
}

AboutDialog::~AboutDialog()
{
  delete ui;
}
