#ifndef DEEPLEARNINGSERVERPANEL_H
#define DEEPLEARNINGSERVERPANEL_H

#include "DeepLearningSegmentationModel.h"
#include "SNAPComponent.h"
#include <QWidget>

namespace Ui
{
class DeepLearningServerPanel;
}

class DeepLearningServerEditor;

class DeepLearningServerPanel : public SNAPComponent
{
  Q_OBJECT

public:
  explicit DeepLearningServerPanel(QWidget *parent = nullptr);
  ~DeepLearningServerPanel();

  void SetModel(DeepLearningSegmentationModel *model);

private slots:
  virtual void onModelUpdate(const EventBucket &bucket) override;
  void checkServerStatus();
  void updateServerStatus();

  void onServerEditorFinished(int accepted);

  void on_btnNew_clicked();

  void on_btnEdit_clicked();

  void on_btnDelete_clicked();

private:
  Ui::DeepLearningServerPanel *ui;
  DeepLearningSegmentationModel *m_Model;

  SmartPtr<DeepLearningServerPropertiesModel> m_CurrentEditorModel;
  bool m_IsNewServer;
  DeepLearningServerEditor *m_EditorDialog = nullptr;

  // How often to perform a status check
  constexpr static unsigned int STATUS_CHECK_INIT_DELAY_MS=500, STATUS_CHECK_FREQUENCY_MS=10000;
  QTimer *m_StatusCheckTimer;
};

#endif // DEEPLEARNINGSERVERPANEL_H
