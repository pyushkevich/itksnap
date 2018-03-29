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
  makeCoupling(ui->outServiceDesc, m_Model->GetServiceDescriptionModel());

  // Listen for server changes
  LatentITKEventNotifier::connect(
        model, DistributedSegmentationModel::ServerChangeEvent(),
        this, SLOT(onModelUpdate(const EventBucket &)));

  // Listen for server changes
  LatentITKEventNotifier::connect(
        model, DistributedSegmentationModel::ServiceChangeEvent(),
        this, SLOT(onModelUpdate(const EventBucket &)));

}

void DistributedSegmentationDialog::onModelUpdate(const EventBucket &bucket)
{
  if(bucket.HasEvent(DistributedSegmentationModel::ServerChangeEvent()))
    {
    // The server has changed. We should launch a separate job to connect to the
    // server, get the list of services, and update the status.
    QFuture<dss_model::StatusCheckResponse> future =
        QtConcurrent::run(DistributedSegmentationModel::AsyncCheckStatus,
                          m_Model->GetURL(""), m_Model->GetToken());

    QFutureWatcher<dss_model::StatusCheckResponse> *watcher =
        new QFutureWatcher<dss_model::StatusCheckResponse>();
    connect(watcher, SIGNAL(finished()), this, SLOT(updateServerStatus()));
    watcher->setFuture(future);
    }
  if(bucket.HasEvent(DistributedSegmentationModel::ServiceChangeEvent()))
    {
    // The service has changed. Launch a separate job to get the service details
    QFuture<dss_model::ServiceDetailResponse> future =
        QtConcurrent::run(DistributedSegmentationModel::AsyncGetServiceDetails,
                          m_Model->GetCurrentServiceGitHash());

    QFutureWatcher<dss_model::ServiceDetailResponse> *watcher =
        new QFutureWatcher<dss_model::ServiceDetailResponse>();
    connect(watcher, SIGNAL(finished()), this, SLOT(updateServiceDetail()));
    watcher->setFuture(future);
    }
}

void DistributedSegmentationDialog::updateServerStatus()
{
  QFutureWatcher<dss_model::StatusCheckResponse> *watcher =
      dynamic_cast<QFutureWatcher<dss_model::StatusCheckResponse> *>(this->sender());

  m_Model->ApplyStatusCheckResponse(watcher->result());

  delete watcher;
}

void DistributedSegmentationDialog::updateServiceDetail()
{
  QFutureWatcher<dss_model::ServiceDetailResponse> *watcher =
      dynamic_cast<QFutureWatcher<dss_model::ServiceDetailResponse> *>(this->sender());

  m_Model->ApplyServiceDetailResponse(watcher->result());

  delete watcher;
}

void DistributedSegmentationDialog::on_btnGetToken_clicked()
{
  // Open the web browsersdf
  QDesktopServices::openUrl(QUrl(m_Model->GetURL("token").c_str()));
}
