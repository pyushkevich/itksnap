#include "SnakeWizardPanel.h"
#include "ui_SnakeWizardPanel.h"
#include "SpeedImageDialog.h"
#include "GlobalUIModel.h"
#include "SnakeWizardModel.h"
#include "QtComboBoxCoupling.h"

Q_DECLARE_METATYPE(SnakeType)

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
  // Store and pass on the models
  m_ParentModel = model;
  m_Model = model->GetSnakeWizardModel();
  m_SpeedDialog->SetModel(m_Model);

  // Make associations
  makeCoupling(ui->inSnakeType, m_Model->GetSnakeTypeModel());
}
