#include "DeepLearningServerEditor.h"
#include "ui_DeepLearningServerEditor.h"


#include "QtComboBoxCoupling.h"
#include "QtAbstractButtonCoupling.h"
#include "QtLabelCoupling.h"
#include "QtLineEditCoupling.h"
#include "QtSpinBoxCoupling.h"
#include "QtCheckBoxCoupling.h"
#include "QtWidgetActivator.h"

DeepLearningServerEditor::DeepLearningServerEditor(QWidget *parent)
  : QDialog(parent)
  , ui(new Ui::DeepLearningServerEditor)
{
  ui->setupUi(this);
  this->setModal(true);
}

DeepLearningServerEditor::~DeepLearningServerEditor() { delete ui; }

void
DeepLearningServerEditor::SetModel(DeepLearningServerPropertiesModel *model)
{
  m_Model = model;
  makeCoupling(ui->inNickname, m_Model->GetNicknameModel());
  makeCoupling(ui->inHostname, m_Model->GetHostnameModel());
  makeCoupling(ui->inPort, m_Model->GetPortModel());
  makeCoupling(ui->chkTunnel, m_Model->GetUseSSHTunnelModel());

  makeCoupling(ui->outURL, m_Model->GetFullURLModel());
}
