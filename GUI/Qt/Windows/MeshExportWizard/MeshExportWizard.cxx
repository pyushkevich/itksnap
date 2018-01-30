#include "MeshExportWizard.h"
#include "ui_MeshExportWizard.h"

MeshExportWizard::MeshExportWizard(QWidget *parent) :
  QWizard(parent),
  ui(new Ui::MeshExportWizard)
{
  // Use parent's double buffering attributes
  this->setAttribute(Qt::WA_PaintOnScreen, parent->testAttribute(Qt::WA_PaintOnScreen));

  ui->setupUi(this);
  this->setWizardStyle(QWizard::ClassicStyle);
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
