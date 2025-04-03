#include "DeepLearningServerPanel.h"
#include "DeepLearningServerEditor.h"
#include "ui_DeepLearningServerPanel.h"

#include "QtComboBoxCoupling.h"
#include "QtAbstractButtonCoupling.h"
#include "QtLabelCoupling.h"
#include "QtWidgetActivator.h"
#include "GlobalUIModel.h"
#include <QtConcurrent/qtconcurrentrun.h>
#include <QtCore/qfuturewatcher.h>


/**
 * Traits for mapping status codes to a label
 */
template<>
class DefaultWidgetValueTraits<dls_model::ConnectionStatus, QLabel>
  : public WidgetValueTraitsBase<dls_model::ConnectionStatus, QLabel *>
{
public:
  typedef dls_model::ConnectionStatus TAtomic;

  virtual TAtomic GetValue(QLabel *w)
  {
    return dls_model::ConnectionStatus();
  }

  virtual void SetValue(QLabel *w, const TAtomic &value)
  {
    switch(value.status)
    {
      case dls_model::CONN_NO_SERVER:
        w->setText("Server not configured");
        w->setStyleSheet("color: darkred; font-weight: bold;");
        break;
      case dls_model::CONN_CHECKING:
        w->setText("Checking status ...");
        w->setStyleSheet("color: black; font-weight: bold;");
        break;
      case dls_model::CONN_NOT_CONNECTED:
        w->setText(QString("Not connected: %1").arg(from_utf8(value.error_message)));
        w->setStyleSheet("color: darkred; font-weight: bold;");
        break;
      case dls_model::CONN_CONNECTED:
        w->setText(QString("Connected, server version: %1").arg(from_utf8(value.server_version)));
        w->setStyleSheet("color: darkgreen; font-weight: bold;");
        break;
    }
  }
};


DeepLearningServerPanel::DeepLearningServerPanel(QWidget *parent)
  : SNAPComponent(parent)
  , ui(new Ui::DeepLearningServerPanel)
{
  ui->setupUi(this);

  // Update check timer
  m_StatusCheckTimer = new QTimer(this);
  connect(m_StatusCheckTimer, &QTimer::timeout, this, &DeepLearningServerPanel::checkServerStatus);

}

DeepLearningServerPanel::~DeepLearningServerPanel() { delete ui; }


void
DeepLearningServerPanel::SetModel(DeepLearningSegmentationModel *model)
{
  m_Model = model;

  makeCoupling(ui->inActiveServer, m_Model->GetServerURLModel());
  makeCoupling(ui->lblStatus, m_Model->GetServerStatusModel());

  makeWidgetVisibilityCoupling(ui->btnEdit, m_Model->GetServerConfiguredModel());
  makeWidgetVisibilityCoupling(ui->btnDelete, m_Model->GetServerConfiguredModel());

  // Listen for server changes
  LatentITKEventNotifier::connect(m_Model,
                                  DeepLearningSegmentationModel::ServerChangeEvent(),
                                  this,
                                  SLOT(onModelUpdate(const EventBucket &)));

  // Set up the timer
  m_StatusCheckTimer->start(STATUS_CHECK_INIT_DELAY_MS);
}

void
DeepLearningServerPanel::onModelUpdate(const EventBucket &bucket)
{
  if(bucket.HasEvent(DeepLearningSegmentationModel::ServerChangeEvent()))
  {
    // The server has changed. We should launch a separate job to connect to the
    // server, get the list of services, and update the status.
    qDebug() << "EVENT-PROMPTED SERVER check for " << m_Model->GetServerURL();
    m_Model->SetServerStatus(dls_model::ConnectionStatus(dls_model::CONN_CHECKING));
    checkServerStatus();
  }
}

void DeepLearningServerPanel::checkServerStatus()
{
  // Stop the timer if active
  if(m_StatusCheckTimer->isActive())
    m_StatusCheckTimer->stop();

  // The server has changed. We should launch a separate job to connect to the
  // server, get the list of services, and update the status.
  using StatusCheck = DeepLearningSegmentationModel::StatusCheck;

  QFuture<StatusCheck> future =
    QtConcurrent::run(DeepLearningSegmentationModel::AsyncCheckStatus, m_Model->GetURL(""));

  QFutureWatcher<StatusCheck> *watcher = new QFutureWatcher<StatusCheck>();
  connect(watcher, SIGNAL(finished()), this, SLOT(updateServerStatus()));
  watcher->setFuture(future);
}

void DeepLearningServerPanel::updateServerStatus()
{
  using StatusCheck = DeepLearningSegmentationModel::StatusCheck;
  QFutureWatcher<StatusCheck> *watcher = dynamic_cast<QFutureWatcher<StatusCheck> *>(this->sender());

  m_Model->ApplyStatusCheckResponse(watcher->result());
  qDebug() << "SERVER " << m_Model->GetServerURL() << " status " << m_Model->GetServerStatus().status;

  delete watcher;

  // Schedule another status check
  // TODO: this can cause more than one status check per second
  m_StatusCheckTimer->start(STATUS_CHECK_FREQUENCY_MS);
}


void
DeepLearningServerPanel::onServerEditorFinished(int accepted)
{
  if(accepted)
  {
    m_Model->UpdateServerProperties(m_CurrentEditorModel, m_IsNewServer);
  }
  m_CurrentEditorModel = nullptr;
  m_EditorDialog->close();
  delete m_EditorDialog;
  m_EditorDialog = nullptr;
}

void
DeepLearningServerPanel::on_btnNew_clicked()
{
  // Create a new server for editing
  m_CurrentEditorModel = DeepLearningServerPropertiesModel::New();
  m_IsNewServer = true;
  m_EditorDialog = new DeepLearningServerEditor(this);
  connect(m_EditorDialog,
          &DeepLearningServerEditor::finished,
          this,
          &DeepLearningServerPanel::onServerEditorFinished);
  m_EditorDialog->SetModel(m_CurrentEditorModel);
  m_EditorDialog->open();
}

void
DeepLearningServerPanel::on_btnEdit_clicked()
{
  // Create a new server for editing
  m_CurrentEditorModel = m_Model->GetServerProperties();
  if(m_CurrentEditorModel)
  {
    m_IsNewServer = false;
    m_EditorDialog = new DeepLearningServerEditor(this);
    connect(m_EditorDialog, &DeepLearningServerEditor::finished, this, &DeepLearningServerPanel::onServerEditorFinished);
    m_EditorDialog->SetModel(m_CurrentEditorModel);
    m_EditorDialog->open();
  }
}

void
DeepLearningServerPanel::on_btnDelete_clicked()
{
  m_Model->DeleteCurrentServer();
}
