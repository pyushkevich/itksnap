#include "SplashPanel.h"
#include "ui_SplashPanel.h"

SplashPanel::SplashPanel(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::SplashPanel)
{
  ui->setupUi(this);
}

SplashPanel::~SplashPanel()
{
  delete ui;
}
