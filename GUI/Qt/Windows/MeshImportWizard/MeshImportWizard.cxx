#include "MeshImportWizard.h"
#include "ui_MeshImportWizard.h"

MeshImportWizard::MeshImportWizard(QWidget *parent) :
  QWizard(parent),
  ui(new Ui::MeshImportWizard)
{
  ui->setupUi(this);
}

MeshImportWizard::~MeshImportWizard()
{
  delete ui;
}
