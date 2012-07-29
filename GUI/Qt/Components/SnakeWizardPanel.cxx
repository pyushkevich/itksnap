#include "SnakeWizardPanel.h"
#include "ui_SnakeWizardPanel.h"

SnakeWizardPanel::SnakeWizardPanel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SnakeWizardPanel)
{
    ui->setupUi(this);
}

SnakeWizardPanel::~SnakeWizardPanel()
{
    delete ui;
}

void SnakeWizardPanel::on_pushButton_4_clicked()
{
  ui->stack->setCurrentIndex((ui->stack->currentIndex() + 1) % 3);
}
