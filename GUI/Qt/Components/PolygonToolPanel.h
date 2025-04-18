#ifndef POLYGONTOOLPANEL_H
#define POLYGONTOOLPANEL_H

#include <QWidget>
#include "SNAPComponent.h"

namespace Ui {
  class PolygonToolPanel;
}

class GlobalUIModel;
class DeepLearningInfoDialog;

class PolygonToolPanel : public SNAPComponent
{
  Q_OBJECT

public:
  explicit PolygonToolPanel(QWidget *parent = 0);
  ~PolygonToolPanel();

  void SetModel(GlobalUIModel *model);
private slots:
  virtual void onModelUpdate(const EventBucket &bucket) override;
  void         on_btnConfigDL_clicked();
  void         onDLInfoDialogFinished(int result);

private:
  Ui::PolygonToolPanel   *ui;
  DeepLearningInfoDialog *m_DLInfoDialog;
  GlobalUIModel          *m_Model;
};

#endif // POLYGONTOOLPANEL_H
