#ifndef LABELSELECTIONPOPUP_H
#define LABELSELECTIONPOPUP_H

#include "SNAPComponent.h"

namespace Ui {
class LabelSelectionPopup;
}

class GlobalUIModel;
class QToolBar;
class EventBucket;

class LabelSelectionPopup : public SNAPComponent
{
  Q_OBJECT
  
public:
  explicit LabelSelectionPopup(QWidget *parent = 0);
  ~LabelSelectionPopup();

  void SetModel(GlobalUIModel *model);

public slots:

  virtual void onModelUpdate(const EventBucket &bucket);

  virtual void onForegroundToolbarAction(QAction *action);
  virtual void onBackgroundToolbarAction(QAction *action);

private:
  Ui::LabelSelectionPopup *ui;

  GlobalUIModel *m_Model;

  QToolBar *m_ToolBarFore, *m_ToolBarBack;

  void UpdateContents();
};

#endif // LABELSELECTIONPOPUP_H
