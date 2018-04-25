#include "DownloadTicketDialog.h"
#include "ui_DownloadTicketDialog.h"
#include "DistributedSegmentationModel.h"

#include <QtComboBoxCoupling.h>

Q_DECLARE_METATYPE(DistributedSegmentationModel::DownloadAction)

DownloadTicketDialog::DownloadTicketDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::DownloadTicketDialog)
{
  ui->setupUi(this);

}

DownloadTicketDialog::~DownloadTicketDialog()
{
  delete ui;
}

void DownloadTicketDialog::SetModel(DistributedSegmentationModel *model)
{
  // Store the model
  m_Model = model;

  // Initialize the file panel
  ui->filePanel->initializeForSaveFile(
        m_Model->GetParent(), "Download location:", "", "ITK-SNAP Workspaces (*.itksnap)", true);
  ui->filePanel->setAllowCreateDir(true);

  // Hook up the download mode
  makeCoupling(ui->inOpenMode, m_Model->GetDownloadActionModel());
}

void DownloadTicketDialog::InitializeToSuggestedFilename()
{
  ui->filePanel->setFilename(from_utf8(m_Model->SuggestDownloadFilename()));
}

QString DownloadTicketDialog::showDialog(QWidget *parent, DistributedSegmentationModel *model)
{
  // Create a dialog and initialize it
  DownloadTicketDialog *dd = new DownloadTicketDialog(parent);
  dd->SetModel(model);

  // Set the suggested filename
  dd->InitializeToSuggestedFilename();

  // Show the dialog
  QString result = (dd->exec() == QDialog::Accepted)
                   ? dd->ui->filePanel->absoluteFilename()
                   : QString();

  // Close the dialog
  dd->close();

  // Delete the dialog
  dd->deleteLater();

  // Return the result
  return result;
}
