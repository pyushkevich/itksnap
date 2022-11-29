#include "MeshImportWizard.h"
#include "ui_MeshImportWizard.h"
#include "MeshImportFileSelectionPage.h"
#include "MeshImportModel.h"
#include <QAbstractButton>

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

QMessageBox *
MeshImportWizard
::CreateLoadToNewLayerMessageBox(QWidget *parent, unsigned int displayTP)
{
  QMessageBox *msgBox = new QMessageBox(parent);
  msgBox->setText("Load file into a new mesh layer?");
  std::ostringstream oss;
  oss << "The mesh will be loaded to the current time point ("
      << displayTP
      << ") in a new layer.";
  msgBox->setInformativeText(oss.str().c_str());
  msgBox->setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
  msgBox->setDefaultButton(QMessageBox::Ok);
  // for testing purpose
  msgBox->button(QMessageBox::Ok)->setObjectName("btnOK");
  msgBox->button(QMessageBox::Cancel)->setObjectName("btnCancel");
  return msgBox;
}

QMessageBox *
MeshImportWizard
::CreateLoadToTimePointMessageBox(QWidget *parent, unsigned int displayTP)
{
  QMessageBox *msgBox = new QMessageBox(parent);
  std::ostringstream oss;
  oss << "Load mesh file to the current time point (" << displayTP << ")?";
  msgBox->setText(oss.str().c_str());
  msgBox->setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
  msgBox->setDefaultButton(QMessageBox::Ok);
  // for testing purpose
  msgBox->button(QMessageBox::Ok)->setObjectName("btnOK");
  msgBox->button(QMessageBox::Cancel)->setObjectName("btnCancel");
  return msgBox;
}

