#ifndef PAINTBRUSHTOOLPANEL_H
#define PAINTBRUSHTOOLPANEL_H

#include <QWidget>
#include "SNAPComponent.h"

class PaintbrushSettingsModel;
class DeepLearningInfoDialog;

namespace Ui {
class PaintbrushToolPanel;
}

class PaintbrushToolPanel : public SNAPComponent
{
  Q_OBJECT
  
public:
  explicit PaintbrushToolPanel(QWidget *parent = 0);
  ~PaintbrushToolPanel();

  void SetModel(PaintbrushSettingsModel *model);

    
private slots:
  virtual void onModelUpdate(const EventBucket &bucket) override;

  void on_actionBrushStyle_triggered();

  void on_btnConfigDL_clicked();

  void onDLInfoDialogFinished(int result);

private:
  Ui::PaintbrushToolPanel *ui;
  PaintbrushSettingsModel *m_Model;
  DeepLearningInfoDialog *m_DLInfoDialog;
};

#endif // PAINTBRUSHTOOLPANEL_H
