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
#include "LayoutReminderDialog.h"
#include "MeshImportModel.h"
#include <QtWidgetActivator.h>
#include "LatentITKEventNotifier.h"
#include "AllPurposeProgressAccumulator.h"
#include "MeshImportWizard.h"
#include "ImageIOWizardModel.h"
#include "ImageIOWizard.h"
#include "GuidedMeshIO.h"
#include <itkImageIOBase.h>
#include <QFileInfo>

DropActionDialog::DropActionDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::DropActionDialog)
{
  ui->setupUi(this);
}

DropActionDialog::~DropActionDialog()
{
  delete ui;
}

void DropActionDialog::SetDroppedFilename(QString name)
{
  ui->outFilename->setText(name);

  bool isWorkspace4D = m_Model->GetDriver()->GetNumberOfTimePoints() > 1;

  // Check if the file can be loaded as mesh
  bool isPolyData = GuidedMeshIO::IsFilePolyData(to_utf8(name).c_str());

  if (isPolyData)
    {
    QFileInfo fileinfo(name);
    auto ext = fileinfo.completeSuffix();
    auto fmt = GuidedMeshIO::GetFormatByExtension(ext.toStdString());

    if (GuidedMeshIO::can_read(fmt))
      {
      this->SetIncludeMeshOptions(true);
      }
    else
      {
      QMessageBox msgBox;
      std::ostringstream oss;
      oss << "Unsupported mesh file type (" << ext.toStdString() << ")!";
      msgBox.setText(oss.str().c_str());
      msgBox.exec();
      this->reject();
      return;
      }
    }
  else
    {
    this->SetIncludeMeshOptions(false);
    // Run segmentation 3d & 4d check
    auto io = GuidedNativeImageIO::New();
    Registry dummyReg;
    io->ReadNativeImageHeader(name.toStdString().c_str(), dummyReg);
    auto header = io->GetIOBase();
    QString btnLoadSegText("Load as Segmentation");
    QString btnLoadSegToolTip("This will replace the current segmentation image with the dropped image.");

    if (header->GetNumberOfDimensions() < 4 && isWorkspace4D)
      {
      btnLoadSegText = QString("Load as Segmentation in Time Point");
      btnLoadSegToolTip = QString("This will replace the segmentation in current time point");
      }

    ui->btnLoadSegmentation->setText(btnLoadSegText);
    ui->btnLoadSegmentation->setToolTip(btnLoadSegToolTip);
    }
}

void DropActionDialog::SetModel(GlobalUIModel *model)
{
  m_Model = model;

  activateOnFlag(ui->btnLoadMeshToTP, m_Model, UIF_MESH_TP_LOADABLE,
                 QtWidgetActivator::HideInactive);

  LatentITKEventNotifier::connect(
        m_Model, ActiveLayerChangeEvent(),
        this, SLOT(onModelUpdate(EventBucket)));

}

void
DropActionDialog
::onModelUpdate(const EventBucket &bucket)
{
  if (bucket.HasEvent(ActiveLayerChangeEvent()))
    this->update();
}

void
DropActionDialog
::SetIncludeMeshOptions(bool include_mesh)
{
  if (!include_mesh)
    {
    ui->btnLoadMeshAsLayer->hide();
    ui->btnLoadMeshToTP->hide();
    }
  else
    {
    ui->btnLoadMeshAsLayer->show();
    }

}

void DropActionDialog::InitialLoad(QString name)
{
  LoadMainImage(name);
}

void DropActionDialog::LoadMainImage(QString name)
{
  this->SetDroppedFilename(name);
  this->on_btnLoadMain_clicked();
}

void DropActionDialog::LoadMesh(QString name)
{
  this->SetDroppedFilename(name);
  this->on_btnLoadMeshAsLayer_clicked();
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

void DropActionDialog::on_btnLoadMeshAsLayer_clicked()
{
  // Get file extension
  std::string fn = ui->outFilename->text().toStdString();
  // Get the file extension with the dot. e.g. ".vtk"
  std::string ext = fn.substr(fn.find_last_of("."));
  std::vector<std::string> fn_list { fn };

  // Create a message box reminding user
  unsigned int displayTP = m_Model->GetDriver()->GetCursorTimePoint() + 1; // always display 1-based time point
  QMessageBox *msgBox = MeshImportWizard::CreateLoadToNewLayerMessageBox(this, displayTP);
  int ret = msgBox->exec();
  delete msgBox;

  switch (ret)
    {
    case QMessageBox::Ok:
      {
      auto fmt = GuidedMeshIO::GetFormatByExtension(ext);
      if (fmt != GuidedMeshIO::FORMAT_COUNT)
        {
          auto model = m_Model->GetMeshImportModel();
          model->Load(fn_list, fmt, displayTP);
        }
      this->accept();
      return;
      }
    case QMessageBox::Cancel:
    default:
      {
      this->reject();
      return;
      }
    }
}

void DropActionDialog::on_btnLoadMeshToTP_clicked()
{
  // Get file extension
  std::string fn = ui->outFilename->text().toStdString();

  unsigned int displayTP = m_Model->GetDriver()->GetCursorTimePoint() + 1; // always display 1-based tp index
  QMessageBox *box = MeshImportWizard::CreateLoadToTimePointMessageBox(this, displayTP);
  int ret = box->exec();
  delete box;

  switch (ret)
    {
    case QMessageBox::Ok:
      {
      auto model = m_Model->GetMeshImportModel();
      model->LoadToTP(fn.c_str(), GuidedMeshIO::GetFormatByFilename((fn.c_str())));
      this->accept();
      return;
      }
    case QMessageBox::Cancel:
    default:
      {
      this->reject();
      return;
      }
    }
}

void DropActionDialog::on_btnLoadSegmentation_clicked()
{
  SmartPtr<LoadSegmentationImageDelegate> del = LoadSegmentationImageDelegate::New();
  del->Initialize(m_Model->GetDriver());

  auto io = GuidedNativeImageIO::New();

  // if incoming load can overwrite unsaved changes, prompt user for saving
  if (del->CanLoadOverwriteUnsavedChanges(io, to_utf8(ui->outFilename->text())) &&
      !SaveModifiedLayersDialog::PromptForUnsavedSegmentationChanges(m_Model))
    return;

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

void DropActionDialog::LoadCommon(AbstractOpenImageDelegate *delegate)
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

		// Show a progress dialog
		auto parentWidget = static_cast<QWidget*>(this->parent());
		using namespace imageiowiz;
		ImageIOProgressDialog::ScopedPointer progress(new ImageIOProgressDialog(parentWidget));
		this->hide();
		progress->display();

		SmartPtr<ImageReadingProgressAccumulator> irProgAccum =
				ImageReadingProgressAccumulator::New();
		irProgAccum->AddObserver(itk::ProgressEvent(), progress->createCommand());

    try
      {
      IRISWarningList warnings;
			m_Model->GetDriver()->OpenImageViaDelegate(file.c_str(), delegate, warnings, &ioHints, irProgAccum);
      this->accept();
      }
    catch(exception &exc)
      {
      progress->close();
      QMessageBox b(this);
      b.setText(QString("Failed to load image %1").arg(ui->outFilename->text()));
      b.setDetailedText(exc.what());
      b.setIcon(QMessageBox::Critical);
      b.exec();
      }
    }

  if (fmt == GuidedNativeImageIO::FORMAT_ECHO_CARTESIAN_DICOM)
    {
      LayoutReminderDialog *lr = new LayoutReminderDialog(this);
      lr->Initialize(m_Model);
      lr->ConditionalExec(LayoutReminderDialog::Echo_Cartesian_Dicom_Loading);
    }
}

