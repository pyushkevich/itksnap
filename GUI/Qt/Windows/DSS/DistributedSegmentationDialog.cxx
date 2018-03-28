#include "DistributedSegmentationDialog.h"
#include "ui_DistributedSegmentationDialog.h"
#include "DistributedSegmentationModel.h"
#include "QtComboBoxCoupling.h"
#include "QtLineEditCoupling.h"
#include "QtLabelCoupling.h"
#include <QDesktopServices>
#include <QUrl>
#include <QtConcurrent>
#include "SNAPQtCommon.h"
#include "Registry.h"

#include "RESTClient.h"
#include "FormattedTable.h"

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
  makeCoupling(ui->outStatus, m_Model->GetServerStatusStringModel());
  makeCoupling(ui->inService, m_Model->GetCurrentServiceModel());

  // Listen for server changes
  LatentITKEventNotifier::connect(
        model, DistributedSegmentationModel::ServerChangeEvent(),
        this, SLOT(onModelUpdate(const EventBucket &)));
}

dss_model::StatusCheckResponse run_check_status(std::string url, std::string token)
{
  dss_model::StatusCheckResponse response;
  response.auth_response.connected = false;
  response.auth_response.authenticated = false;

  // First try to authenticate
  try
    {
    RESTClient rc;
    if(rc.Authenticate(url.c_str(), token.c_str()))
      {
      response.auth_response.connected = true;
      response.auth_response.authenticated = true;
      }
    else
      {
      response.auth_response.connected = true;
      return response;
      }
    }
  catch(...)
    {
    return response;
    }

  try
    {
    // Second, try to get service listing
    RESTClient rc;
    if(rc.Get("api/services"))
      {
      FormattedTable ft;
      ft.ParseCSV(rc.GetOutput());

      for(int i = 0; i < ft.Rows(); i++)
        {
        dss_model::ServiceSummary service;
        service.name = ft(i, 0);
        service.githash = ft(i, 1);
        service.version = ft(i, 2);
        service.desc = ft(i, 3);
        response.service_listing.push_back(service);
        }
      }
    }
  catch(...)
    {
    }

  return response;
}

void DistributedSegmentationDialog::onModelUpdate(const EventBucket &bucket)
{
  if(bucket.HasEvent(DistributedSegmentationModel::ServerChangeEvent()))
    {
    // The server has changed. We should launch a separate job to connect to the
    // server, get the list of services, and update the status.
    QFuture<dss_model::StatusCheckResponse> future =
        QtConcurrent::run(run_check_status, m_Model->GetURL(""), m_Model->GetToken());

    QFutureWatcher<dss_model::StatusCheckResponse> *watcher =
        new QFutureWatcher<dss_model::StatusCheckResponse>();
    connect(watcher, SIGNAL(finished()), this, SLOT(updateServerStatus()));
    watcher->setFuture(future);
    }
}

void DistributedSegmentationDialog::updateServerStatus()
{
  QFutureWatcher<dss_model::StatusCheckResponse> *watcher =
      dynamic_cast<QFutureWatcher<dss_model::StatusCheckResponse> *>(this->sender());
  dss_model::StatusCheckResponse result = watcher->result();

  if(result.auth_response.connected)
    {
    if(result.auth_response.authenticated)
      m_Model->SetServerStatus(DistributedSegmentationModel::CONNECTED_AUTHORIZED);
    else
      m_Model->SetServerStatus(DistributedSegmentationModel::CONNECTED_NOT_AUTHORIZED);
    }
  else
    {
    m_Model->SetServerStatus(DistributedSegmentationModel::NOT_CONNECTED);
    }

  m_Model->SetServiceListing(result.service_listing);

  delete watcher;
}

void DistributedSegmentationDialog::on_btnGetToken_clicked()
{
  // Open the web browsersdf
  QDesktopServices::openUrl(QUrl(m_Model->GetURL("token").c_str()));
}
