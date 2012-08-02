#include "SnakeWizardPanel.h"
#include "ui_SnakeWizardPanel.h"
#include "SpeedImageDialog.h"
#include "GlobalUIModel.h"

SnakeWizardPanel::SnakeWizardPanel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SnakeWizardPanel)
{
  ui->setupUi(this);

  m_SpeedDialog = new SpeedImageDialog(this);
}

SnakeWizardPanel::~SnakeWizardPanel()
{
  delete ui;
}

void SnakeWizardPanel::on_btnPreprocess_clicked()
{
  // Show the appropriately configured preprocessing dialog
  m_SpeedDialog->exec();
}

void SnakeWizardPanel::SetModel(GlobalUIModel *model)
{
  m_ParentModel = model;
  m_Model = model->GetSnakeWizardModel();
  m_SpeedDialog->SetModel(m_Model);
}
