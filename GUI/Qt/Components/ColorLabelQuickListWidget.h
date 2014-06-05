#ifndef COLORLABELQUICKLISTWIDGET_H
#define COLORLABELQUICKLISTWIDGET_H

#include "SNAPCommon.h"
#include "SNAPComponent.h"

class QToolBar;
class GlobalUIModel;
class QAction;
class QActionGroup;

/**
 * This is a widget holding a toolbar with a list of buttons, each corresponding
 * to a color label. When the user clicks the buttons, that label is assigned
 * to be the foreground label. Right clicking the label gives a context menu.
 */
class ColorLabelQuickListWidget : public SNAPComponent
{
  Q_OBJECT
public:
  explicit ColorLabelQuickListWidget(QWidget *parent = 0);
  
  /** Initialize the widget by assigning it a model */
  void SetModel(GlobalUIModel *model);

  /** Set the number of labels to display */
  void setMaximumLabelCount(int value);

signals:

  void actionTriggered(QAction *);
  
public slots:

protected:

  // The widget contains a toolbar with a list of actions
  QToolBar *m_Toolbar;

  // An action group controlling action behavior
  QActionGroup *m_ActionGroup;

  // A pointer to the model
  GlobalUIModel *m_Model;

  // The number of labels to display
  int m_MaximumLabelCount;

  // Handle an update from the model
  virtual void onModelUpdate(const EventBucket &bucket);

  // Populate the toolbar from the model
  void UpdateToolbar();

};

#endif // COLORLABELQUICKLISTWIDGET_H
