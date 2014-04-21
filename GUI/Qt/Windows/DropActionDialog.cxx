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

void DropActionDialog::LoadCommon(AbstractLoadImageDelegate *delegate)
{
  std::string file = to_utf8(ui->outFilename->text());
  QtCursorOverride c(Qt::WaitCursor);
  try
    {
    IRISWarningList warnings;
    m_Model->GetDriver()->LoadImageViaDelegate(file.c_str(), delegate, warnings);
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
