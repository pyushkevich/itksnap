#include "DeepLearningServerPanel.h"
#include "DeepLearningServerEditor.h"
#include "ui_DeepLearningServerPanel.h"

#include "QtComboBoxCoupling.h"
#include "QtAbstractButtonCoupling.h"
#include "QtLabelCoupling.h"
#include "QtWidgetActivator.h"
#include "QtLocalDeepLearningServerDelegate.h"
#include "GlobalUIModel.h"
#include <QtConcurrent>
#include <QtCore>

#include <QTcpSocket>
#include <QTcpServer>
#include <SSHTunnel.h>
#include <libssh/libssh.h>
#include "SSHTunnelWorkerThread.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QShowEvent>
#include <QFont>
#include <QPixmap>
#include <QPainter>
#include <QPaintDevice>

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
      case dls_model::CONN_TUNNEL_ESTABLISHING:
        w->setText("Opening tunnel to SSH server ...");
        w->setStyleSheet("color: black; font-weight: bold;");
        break;
      case dls_model::CONN_TUNNEL_FAILED:
        w->setText(QString("SSH tunnel failure: %1").arg(from_utf8(value.error_message)));
        w->setStyleSheet("color: darkred; font-weight: bold;");
        break;
      case dls_model::CONN_CHECKING:
        w->setText("Establishing connection ...");
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

class DeepLearningServerPropertiesDescriptionTraits
{
public:
  using Value = SmartPtr<DeepLearningServerPropertiesModel>;
  static QString GetText(int row, const Value &m)
  {
    return QString::fromStdString(m->GetDisplayName());
  }

  static QIcon GetIcon(int row, const Value &m)
  {
    return QIcon::fromTheme(m->GetRemoteConnection() ? QIcon::ThemeIcon::NetworkWireless : QIcon::ThemeIcon::DocumentOpen);
  }

  static QVariant GetIconSignature(int, const Value &)
  {
    return QVariant(0);
  }
};

template <>
class DefaultComboBoxRowTraits<int, DeepLearningServerPropertiesDescriptionTraits::Value>
  : public TextAndIconComboBoxRowTraits<int, DeepLearningServerPropertiesDescriptionTraits::Value, DeepLearningServerPropertiesDescriptionTraits>
{
};


DeepLearningServerPanel::DeepLearningServerPanel(QWidget *parent)
  : SNAPComponent(parent)
  , ui(new Ui::DeepLearningServerPanel)
{
  ui->setupUi(this);

  m_LocalDeepLearningServerDelegate = new QtLocalDeepLearningServerDelegate(this, ui->txtEditServerLog);

  // Update check timer
  m_StatusCheckTimer = new QTimer(this);
  connect(m_StatusCheckTimer, &QTimer::timeout, this, &DeepLearningServerPanel::checkServerStatus);

}

DeepLearningServerPanel::~DeepLearningServerPanel()
{
  delete ui;
  m_StatusCheckTimer->stop();
  removeSSHTunnel();
}


void
DeepLearningServerPanel::SetModel(DeepLearningSegmentationModel *model)
{
  m_Model = model;

  makeCoupling(ui->inActiveServer, m_Model->GetServerModel());
  makeCoupling(ui->lblStatus, m_Model->GetServerStatusModel());

  makeWidgetVisibilityCoupling(ui->btnEdit, m_Model->GetIsServerConfiguredModel());
  makeWidgetVisibilityCoupling(ui->btnDelete, m_Model->GetIsServerConfiguredModel());

  // Assign our delegate to the model
  // Delegate for running itksnap_dls locally
  m_Model->SetLocalServerDelegate(m_LocalDeepLearningServerDelegate);



  // Listen for server changes
  LatentITKEventNotifier::connect(m_Model,
                                  DeepLearningSegmentationModel::ServerChangeEvent(),
                                  this,
                                  SLOT(onModelUpdate(const EventBucket &)));

  // In principle, this is when we should launch resetConnection, but since this may
  // start nagging the user with error messages and what not, it is better to wait
  // until the user has interacted with the AI-based paintbrush. So instead, we should
  // get the model to invoke its ServerChangeEvent when either this dialog is shown or
  // when the AI panel is shown.
  if(this->isVisible())
    m_Model->SetIsActive(true);

  if(m_Model->GetServer() >= 0 && m_Model->GetIsActive())
    resetConnection();

  // Set up the timer
  m_StatusCheckTimer->start(STATUS_CHECK_INIT_DELAY_MS);
}

void
DeepLearningServerPanel::showEvent(QShowEvent *event)
{
  m_Model->SetIsActive(true);
}

void
DeepLearningServerPanel::removeSSHTunnel()
{
  if(m_SSHTunnelWorkerThread)
  {
    m_SSHTunnelWorkerThread->terminate();
    m_SSHTunnelWorkerThread = nullptr;
  }
}

void
DeepLearningServerPanel::setupSSHTunnel()
{
  removeSSHTunnel();

  // Create a new tunnel worker
  auto *p = m_Model->GetServerProperties();
  m_SSHTunnelWorkerThread =
    new SSHTunnelWorkerThread(nullptr,
                              QString::fromStdString(p->GetHostname()),
                              p->GetPort(),
                              QString::fromStdString(p->GetSSHUsername()),
                              QString::fromStdString(p->GetSSHPrivateKeyFile()));

  // Listen to events on the thread
  connect(m_SSHTunnelWorkerThread, &QThread::finished, m_SSHTunnelWorkerThread, &QObject::deleteLater);
  connect(m_SSHTunnelWorkerThread, &QObject::destroyed, this, &DeepLearningServerPanel::onSSHTunnelDestroyed);
  connect(m_SSHTunnelWorkerThread, &SSHTunnelWorkerThread::tunnelError, this, &DeepLearningServerPanel::onSSHTunnelCreationFailed);
  connect(m_SSHTunnelWorkerThread, &SSHTunnelWorkerThread::tunnelReady, this, &DeepLearningServerPanel::onSSHTunnelCreated);
  connect(m_SSHTunnelWorkerThread, &SSHTunnelWorkerThread::tunnelPasswordPrompt, this, &DeepLearningServerPanel::onSSHTunnelPasswordPrompt);
  connect(this, &DeepLearningServerPanel::sshPasswordEntered, m_SSHTunnelWorkerThread, &SSHTunnelWorkerThread::passwordResponse);

  // The status is now "establishing tunnel"
  m_Model->SetServerStatus(dls_model::ConnectionStatus(dls_model::CONN_TUNNEL_ESTABLISHING));

  // Start the thread - this means that the server will start connecting
  qDebug() << "Starting SSH tunnel thread for " << p->GetHostname() << " port " << p->GetPort();
  m_SSHTunnelWorkerThread->start();
}

void DeepLearningServerPanel::onSSHTunnelCreated(int localPort)
{
  // Set the proxy port
  std::ostringstream oss;
  oss << "http://localhost:" << localPort;
  m_Model->SetProxyURL(oss.str());

  // Status is now connecting
  m_Model->SetServerStatus(dls_model::ConnectionStatus(dls_model::CONN_CHECKING));
  this->checkServerStatus();
}

void DeepLearningServerPanel::onSSHTunnelCreationFailed(const QString &error)
{
  // Let user know that the tunnel failed
  // QMessageBox::warning(
  //  this, "Failed to create tunnel", QString("Failed to create tunnel: %1").arg(error));

  // Set the proxy URL to none
  m_Model->SetProxyURL(std::string());

  // Pass the error to the panel
  dls_model::ConnectionStatus status(dls_model::CONN_TUNNEL_FAILED);
  status.error_message = error.toStdString();
  m_Model->SetServerStatus(status);

}

void
DeepLearningServerPanel::onSSHTunnelDestroyed(QObject *obj)
{
  if(obj == m_SSHTunnelWorkerThread)
    m_SSHTunnelWorkerThread = nullptr;
}

void
DeepLearningServerPanel::onSSHTunnelPasswordPrompt(SSHTunnel::PromptPasswordInfo pinfo)
{
  bool is_ok;

  QString error = pinfo.error_message.size()
                    ? QString("Error: %1\n\n").arg(pinfo.error_message.c_str())
                    : QString();

  QString password =
    QInputDialog::getText(this,
                          QString("Password requested"),
                          QString("%1SSH server %2 requires password for user %3")
                            .arg(error, pinfo.server.c_str(), pinfo.username.c_str()),
                          QLineEdit::Password,
                          QString(),
                          &is_ok);

  emit(sshPasswordEntered(password, !is_ok));
}


void
DeepLearningServerPanel::onModelUpdate(const EventBucket &bucket)
{
  if(bucket.HasEvent(DeepLearningSegmentationModel::ServerChangeEvent()))
  {
    // The server has changed. We should launch a separate job to connect to the
    // server, get the list of services, and update the status.
    resetConnection();
  }
}

void
DeepLearningServerPanel::resetConnection()
{
  if(!m_Model || !m_Model->GetIsActive())
    return;

  // Start the local server if needed
  m_Model->StartLocalServerIfNeeded();

  // Reset the server status
  m_Model->SetServerStatus(dls_model::ConnectionStatus(dls_model::CONN_CHECKING));
  m_Model->SetProxyURL(std::string());

  if(m_Model->GetIsServerConfigured() && m_Model->GetServerProperties()->GetUseSSHTunnel())
  {
    setupSSHTunnel();
  }
  else
  {
    removeSSHTunnel();
    checkServerStatus();
  }
}

void DeepLearningServerPanel::checkServerStatus()
{
  // Stop the timer if active
  if(m_StatusCheckTimer->isActive())
    m_StatusCheckTimer->stop();

  // If there is no server to connect to, then just exit
  if (!m_Model->GetIsServerConfigured() ||
      m_Model->GetServerProperties()->GetUseSSHTunnel() && m_Model->GetProxyURL().empty())
    return;

  // The server has changed. We should launch a separate job to connect to the
  // server, get the list of services, and update the status.
  using StatusCheck = DeepLearningSegmentationModel::StatusCheck;
  QFuture<StatusCheck> future =
    QtConcurrent::run([this]() -> StatusCheck { return m_Model->AsyncCheckStatus(); });

  QFutureWatcher<StatusCheck> *watcher = new QFutureWatcher<StatusCheck>();
  connect(watcher, SIGNAL(finished()), this, SLOT(updateServerStatus()));
  connect(this, &QObject::destroyed, watcher, &QFutureWatcherBase::cancel);
  watcher->setFuture(future);
}

void DeepLearningServerPanel::updateServerStatus()
{
  using StatusCheck = DeepLearningSegmentationModel::StatusCheck;
  QFutureWatcher<StatusCheck> *watcher = dynamic_cast<QFutureWatcher<StatusCheck> *>(this->sender());
  if(watcher->isCanceled())
    qDebug() << "Canceled watcher triggered signal";

  m_Model->ApplyStatusCheckResponse(watcher->result());
  qDebug() << "SERVER " << m_Model->GetServerDisplayURL() << " status " << m_Model->GetServerStatus().status;

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
DeepLearningServerPanel::ShowEditorDialog()
{
  // Create a new dialog
  m_EditorDialog = new QDialog(this->window());
  m_EditorDialog->setWindowTitle("Deep Learning Extension Server Configuration");
  m_Editor = new DeepLearningServerEditor(m_EditorDialog);
  connect(m_EditorDialog,
          &QDialog::finished,
          this,
          &DeepLearningServerPanel::onServerEditorFinished);

  m_Editor->SetModel(m_CurrentEditorModel);

  QVBoxLayout *lo = new QVBoxLayout(m_EditorDialog);
  QDialogButtonBox *bbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, m_EditorDialog);
  connect(bbox, &QDialogButtonBox::accepted, m_EditorDialog, &QDialog::accept);
  connect(bbox, &QDialogButtonBox::rejected, m_EditorDialog, &QDialog::reject);

  m_EditorDialog->setLayout(lo);
  lo->addWidget(m_Editor);
  lo->addWidget(bbox);
  m_EditorDialog->setModal(true);
  m_EditorDialog->open();
}

void
DeepLearningServerPanel::on_btnNew_clicked()
{
  // Create a new server for editing
  m_CurrentEditorModel = DeepLearningServerPropertiesModel::New();
  m_IsNewServer = true;
  ShowEditorDialog();
}

void
DeepLearningServerPanel::on_btnEdit_clicked()
{
  // Create a new server for editing
  m_CurrentEditorModel = m_Model->GetServerProperties();
  if(m_CurrentEditorModel)
  {
    m_IsNewServer = false;
    ShowEditorDialog();
  }
}

void
DeepLearningServerPanel::on_btnDelete_clicked()
{
  m_Model->DeleteCurrentServer();
}


void
DeepLearningServerPanel::on_btnReconnect_clicked()
{
  this->resetConnection();
}
