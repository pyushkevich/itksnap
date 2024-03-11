#include "LoadTransformationDialog.h"
#include "ui_LoadTransformationDialog.h"
#include "FileChooserPanelWithHistory.h"

#include "SNAPQtCommon.h"

LoadTransformationDialog::LoadTransformationDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::LoadTransformationDialog)
{
  // Use parent's double buffering attributes
  this->setAttribute(Qt::WA_PaintOnScreen, parent->testAttribute(Qt::WA_PaintOnScreen));

  // Set an object name for scripting
  this->setObjectName("dlgLoadTransformation");

  ui->setupUi(this);

  // Connect up
  connect(this, SIGNAL(accepted()), ui->filePanel, SLOT(onFilenameAccept()));
}

LoadTransformationDialog::~LoadTransformationDialog()
{
  delete ui;
}

LoadTransformationDialog::QueryResult
LoadTransformationDialog
::showDialog(QWidget *parent, GlobalUIModel *model)
{
  QueryResult result;

  // Configure the dialog
  LoadTransformationDialog *dialog = new LoadTransformationDialog(parent);
  dialog->ui->filePanel->
      initializeForOpenFile(model, "Transform File", "AffineTransform",
                            "ITK Transform Files (*.txt);; Convert3D Transform Files (*.mat)",
                            QString());
  dialog->setWindowTitle("Open Transform - ITK-SNAP");

  // Launch the dialog
  if(dialog->exec() == QDialog::Accepted)
    {
    result.filename = dialog->ui->filePanel->absoluteFilename();
    result.activeFormat = dialog->ui->filePanel->activeFormat();
    result.compose = dialog->ui->chkCompose->isChecked();
    result.inverse = dialog->ui->chkInverse->isChecked();
    }

  return result;
}


