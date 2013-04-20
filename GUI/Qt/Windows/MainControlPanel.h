#ifndef MAINCONTROLPANEL_H
#define MAINCONTROLPANEL_H

#include <SNAPComponent.h>

class LabelSelectionButton;
class LabelSelectionPopup;

namespace Ui {
class MainControlPanel;
}

class GlobalUIModel;
class MainImageWindow;

class MainControlPanel : public SNAPComponent
{
  Q_OBJECT
  
public:
  explicit MainControlPanel(MainImageWindow *parent = 0);
  ~MainControlPanel();

  void SetModel(GlobalUIModel *model);

protected slots:

  virtual void onModelUpdate(const EventBucket &bucket);

private slots:

  void on_btnCursorInspector_clicked(bool checked);

  void on_btnZoomInspector_clicked(bool checked);

  void on_btnLabelInspector_clicked(bool checked);

  void on_btnDisplayInspector_clicked(bool checked);

  void on_btnSyncInspector_clicked(bool checked);

  void on_btnToolInspector_clicked(bool checked);


private:
  Ui::MainControlPanel *ui;
  GlobalUIModel *m_Model;

  LabelSelectionButton *m_LabelSelectionButton;
  LabelSelectionPopup *m_LabelSelectionPopup;
};

#endif // MAINCONTROLPANEL_H
