#include "DistributedSegmentationDialog.h"
#include "ui_DistributedSegmentationDialog.h"
#include "DistributedSegmentationModel.h"
#include "QtComboBoxCoupling.h"
#include "QtLineEditCoupling.h"
#include <QDesktopServices>
#include <QUrl>
#include "SNAPQtCommon.h"

#include "RESTClient.h"

DistributedSegmentationDialog::DistributedSegmentationDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::DistributedSegmentationDialog)
{
  ui->setupUi(this);
}

DistributedSegmentationDialog::~DistributedSegmentationDialog()
{
  delete ui;
}

void DistributedSegmentationDialog::SetModel(DistributedSegmentationModel *model)
{
  // Store the model
  m_Model = model;

  // Connect the widgets
  makeCoupling(ui->inServer, m_Model->GetServerURLModel());
  makeCoupling(ui->inToken, m_Model->GetTokenModel());

  // Listen for server changes
  LatentITKEventNotifier::connect(
        model, DistributedSegmentationModel::ServerChangeEvent(),
        this, SLOT(onModelUpdate(const EventBucket &)));
}

bool run_check_status(std::string url, std::string token)
{
  try
  {
    RESTClient rc;
    rc.Authenticate(url.c_str(), token.c_str());
    return true;
  }
  catch(...)
  {
    return false;
  }




}

void DistributedSegmentationDialog::onModelUpdate(const EventBucket &bucket)
{
  if(bucket.HasEvent(DistributedSegmentationModel::ServerChangeEvent()))
    {
    // The server has changed. We should launch a separate job to connect to the
    // server, get the list of services, and update the status.
    }


}

void DistributedSegmentationDialog::updateServerStatus()
{

}

void DistributedSegmentationDialog::on_btnGetToken_clicked()
{
  // Open the web browser
  QDesktopServices::openUrl(QUrl(m_Model->GetURL("token").c_str()));
}
