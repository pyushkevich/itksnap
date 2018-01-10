#include "DropActionDialog.h"
#include "ui_DropActionDialog.h"
#include "QtStyles.h"
#include "GlobalUIModel.h"
#include "ImageIODelegates.h"
#include "SystemInterface.h"
#include "QtWarningDialog.h"
#include "QtCursorOverride.h"
#include <QMessageBox>
#include "SNAPQtCommon.h"
#include "MainImageWindow.h"
#include "SaveModifiedLayersDialog.h"
#include "IRISImageData.h"
#include "GuidedNativeImageIO.h"
#include <QTimer>

DropActionDialog::DropActionDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::DropActionDialog)
{
  ui->setupUi(this);

  // Start from scratch
  ApplyCSS(this, ":/root/itksnap.css");

}

DropActionDialog::~DropActionDialog()
{
  delete ui;
}

void DropActionDialog::SetDroppedFilename(QString name)
{
  ui->outFilename->setText(name);
}

void DropActionDialog::SetModel(GlobalUIModel *model)
{
  m_Model = model;
}

void DropActionDialog::LoadMainImage(QString name)
{
  this->SetDroppedFilename(name);
  this->on_btnLoadMain_clicked();
}

void DropActionDialog::on_btnLoadMain_clicked()
{
  // Prompt for unsaved changes before replacing the main image
  if(!SaveModifiedLayersDialog::PromptForUnsavedChanges(m_Model))
    return;

  SmartPtr<LoadMainImageDelegate> del = LoadMainImageDelegate::New();
  del->Initialize(m_Model->GetDriver());
  this->LoadCommon(del);
}

void DropActionDialog::on_btnLoadSegmentation_clicked()
{
  // Prompt for unsaved changes before replacing the segmentation
  if(!SaveModifiedLayersDialog::PromptForUnsavedSegmentationChanges(m_Model))
    return;

  SmartPtr<LoadSegmentationImageDelegate> del = LoadSegmentationImageDelegate::New();
  del->Initialize(m_Model->GetDriver());
  this->LoadCommon(del);
}


void DropActionDialog::on_btnLoadAdditionalSegmentation_clicked()
{
  SmartPtr<LoadSegmentationImageDelegate> del = LoadSegmentationImageDelegate::New();
  del->Initialize(m_Model->GetDriver());
  del->SetAdditiveMode(true);
  this->LoadCommon(del);
}

void DropActionDialog::on_btnLoadOverlay_clicked()
{
  SmartPtr<LoadOverlayImageDelegate > del = LoadOverlayImageDelegate ::New();
  del->Initialize(m_Model->GetDriver());
  this->LoadCommon(del);
}

void DropActionDialog::on_btnLoadNew_clicked()
{
  std::list<std::string> args;
  args.push_back(to_utf8(ui->outFilename->text()));
  try
    {
    m_Model->GetSystemInterface()->LaunchChildSNAPSimple(args);
    this->accept();
    }
  catch(exception &exc)
    {
    QMessageBox b(this);
    b.setText(QString("Failed to launch new ITK-SNAP instance"));
    b.setDetailedText(exc.what());
    b.setIcon(QMessageBox::Critical);
    b.exec();
    }
}

#include "ImageIOWizardModel.h"
#include "ImageIOWizard.h"

void DropActionDialog::LoadCommon(AbstractLoadImageDelegate *delegate)
{
  // File being loaded
  std::string file = to_utf8(ui->outFilename->text());

  // We need to handle the special case when the filename is a DICOM file or
  // a folder containing DICOM files. In this case, instead of just directly
  // opening the file, we use the wizard.

  // Load the settings associated with this file
  Registry regAssoc;
  m_Model->GetDriver()->GetSystemInterface()
      ->FindRegistryAssociatedWithFile(file.c_str(), regAssoc);

  // Get the folder dealing with grey image properties
  Registry &ioHints = regAssoc.Folder("Files.Grey");
  
  // Is this a file with a known format?
  GuidedNativeImageIO::FileFormat fmt = GuidedNativeImageIO::GetFileFormat(ioHints);

  // If not, peek at the header to see if it's DICOM or unknown
  if(fmt == GuidedNativeImageIO::FORMAT_COUNT)
    fmt = GuidedNativeImageIO::GuessFormatForFileName(file, true);

  // If this has been determined to be a DICOM directory, let's go to the DICOM page
  if(fmt == GuidedNativeImageIO::FORMAT_DICOM_DIR || fmt == GuidedNativeImageIO::FORMAT_COUNT)
    {
    // Create the wizard model
    SmartPtr<ImageIOWizardModel> model = ImageIOWizardModel::New();
    model->InitializeForLoad(m_Model, delegate);
    model->SetSuggestedFilename(file);
    model->SetSuggestedFormat(fmt);

    // Execute the IO wizard
    ImageIOWizard wiz(this);
    wiz.SetModel(model);

    // For DICOM we can move ahead to the second ppage. We do this using a singleshot
    // timer
    if(fmt == GuidedNativeImageIO::FORMAT_DICOM_DIR)
      {
      QTimer::singleShot(0, &wiz, SLOT(next()));
      }

    this->accept();
    wiz.exec();
    }
  else
    {
    // Load without the wizard
    QtCursorOverride c(Qt::WaitCursor);
    try
      {
      IRISWarningList warnings;
      m_Model->GetDriver()->LoadImageViaDelegate(file.c_str(), delegate, warnings, &ioHints);
      this->accept();
      }
    catch(exception &exc)
      {
      QMessageBox b(this);
      b.setText(QString("Failed to load image %1").arg(ui->outFilename->text()));
      b.setDetailedText(exc.what());
      b.setIcon(QMessageBox::Critical);
      b.exec();
      }
    }
}

