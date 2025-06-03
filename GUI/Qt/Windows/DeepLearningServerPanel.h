#ifndef DEEPLEARNINGSERVERPANEL_H
#define DEEPLEARNINGSERVERPANEL_H

#include "DeepLearningSegmentationModel.h"
#include "SNAPComponent.h"
#include <QWidget>
#include <QThread>
#include "SSHTunnel.h"

namespace Ui
{
class DeepLearningServerPanel;
}

class DeepLearningServerEditor;
class SSHTunnelWorkerThread;
class QtLocalDeepLearningServerDelegate;

class DeepLearningServerPanel : public SNAPComponent
{
  Q_OBJECT

public:
  explicit DeepLearningServerPanel(QWidget *parent = nullptr);
  ~DeepLearningServerPanel();

  void SetModel(DeepLearningSegmentationModel *model);

  // Handle show event - activate
  void showEvent(QShowEvent *event) override;

private slots:
  virtual void onModelUpdate(const EventBucket &bucket) override;

  /**
   * Reset the existing server connection or start a new connection. This will
   * launch the SSH tunnel if needed and it will start checking connection
   * status at regular intervals. Should be called when the user changes the
   * server or when the user first needs to connect.
   */
  void resetConnection();
  void checkServerStatus();
  void updateServerStatus();

  void onSSHTunnelCreated(int local_port);

  void onSSHTunnelCreationFailed(const QString &error);

  void onSSHTunnelDestroyed(QObject *obj);

  void onSSHTunnelPasswordPrompt(SSHTunnel::PromptPasswordInfo pinfo);

  void onServerEditorFinished(int accepted);

  void on_btnNew_clicked();

  void on_btnEdit_clicked();

  void on_btnDelete_clicked();

  void on_btnReconnect_clicked();

signals:

  void launchSSHTunnel();

  void sshPasswordEntered(QString password, bool abort);

private:
  Ui::DeepLearningServerPanel *ui;
  DeepLearningSegmentationModel *m_Model;

  SmartPtr<DeepLearningServerPropertiesModel> m_CurrentEditorModel;
  bool m_IsNewServer;
  DeepLearningServerEditor *m_Editor = nullptr;
  QDialog *m_EditorDialog = nullptr;

  // How often to perform a status check
  constexpr static unsigned int STATUS_CHECK_INIT_DELAY_MS=500, STATUS_CHECK_FREQUENCY_MS=10000;
  QTimer *m_StatusCheckTimer;

  // Delegate that handles starting system process
  QtLocalDeepLearningServerDelegate *m_LocalDeepLearningServerDelegate;


  SSHTunnelWorkerThread *m_SSHTunnelWorkerThread = nullptr;
  void setupSSHTunnel();
  void removeSSHTunnel();
  void                   ShowEditorDialog();
};

#endif // DEEPLEARNINGSERVERPANEL_H
