#include "SplashPanel.h"
#include "ui_SplashPanel.h"
#include "SNAPCommon.h"

SplashPanel::SplashPanel(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::SplashPanel)
{
  ui->setupUi(this);
  ui->outVersion->setText(SNAPUISoftVersion);
}

SplashPanel::~SplashPanel()
{
  delete ui;
}
