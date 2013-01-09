#include "DropActionDialog.h"
#include "ui_DropActionDialog.h"
#include "QtStyles.h"
#include "GlobalUIModel.h"
#include "ImageIODelegates.h"
#include "SystemInterface.h"
#include "QtWarningDialog.h"
#include "QtCursorOverride.h"
#include <QMessageBox>

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
  LoadMainImageDelegate del(m_Model, IRISApplication::MAIN_ANY);
  this->LoadCommon(del);
}

void DropActionDialog::on_btnLoadSegmentation_clicked()
{
  LoadSegmentationImageDelegate del(m_Model);
  this->LoadCommon(del);
}

void DropActionDialog::on_btnLoadOverlay_clicked()
{
  LoadOverlayImageDelegate del(m_Model, IRISApplication::MAIN_ANY);
  this->LoadCommon(del);
}

void DropActionDialog::on_btnLoadNew_clicked()
{
  std::list<std::string> args;
  args.push_back(ui->outFilename->text().toStdString());
  try
    {
    m_Model->GetSystemInterface()->LaunchChildSNAP(args);
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

void DropActionDialog::LoadCommon(AbstractLoadImageDelegate &delegate)
{
  QString file = ui->outFilename->text();
  QtCursorOverride c(Qt::WaitCursor);
  try
    {
    IRISWarningList warnings;
    m_Model->LoadImageNonInteractive(file.toStdString().c_str(),
                                     delegate, warnings);
    this->accept();
    }
  catch(exception &exc)
    {
    QMessageBox b(this);
    b.setText(QString("Failed to load image %1").arg(file));
    b.setDetailedText(exc.what());
    b.setIcon(QMessageBox::Critical);
    b.exec();
  }
}
