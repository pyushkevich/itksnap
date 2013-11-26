#include "MeshExportWizard.h"
#include "ui_MeshExportWizard.h"

MeshExportWizard::MeshExportWizard(QWidget *parent) :
  QWizard(parent),
  ui(new Ui::MeshExportWizard)
{
  ui->setupUi(this);
}

MeshExportWizard::~MeshExportWizard()
{
  delete ui;
}

void MeshExportWizard::SetModel(MeshExportModel *model)
{
  ui->pageMode->SetModel(model);
  ui->pageBrowse->SetModel(model);
  m_Model = model;
}
