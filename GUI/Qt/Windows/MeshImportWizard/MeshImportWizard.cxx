#include "MeshImportWizard.h"
#include "ui_MeshImportWizard.h"
#include "MeshImportFileSelectionPage.h"
#include "MeshImportModel.h"

MeshImportWizard::MeshImportWizard(QWidget *parent) :
  QWizard(parent),
  ui(new Ui::MeshImportWizard)
{
  // Set a name for test tracking
  this->setObjectName("wizMeshImport");

  // Use parent's double buffering attributes
  this->setAttribute(Qt::WA_PaintOnScreen, parent->testAttribute(Qt::WA_PaintOnScreen));

  ui->setupUi(this);

  this->setWizardStyle(QWizard::ClassicStyle);
}

void MeshImportWizard::SetModel(MeshImportModel *model)
{
  this->m_Model = model;

  auto filePage = new MeshImportFileSelectionPage(this);
  filePage->SetModel(model);
  this->setPage(0, filePage);
}

MeshImportWizard::~MeshImportWizard()
{
  delete ui;
}
