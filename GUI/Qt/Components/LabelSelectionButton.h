#ifndef LABELSELECTIONBUTTON_H
#define LABELSELECTIONBUTTON_H

#include <QToolButton>
#include <QMenu>
#include <QWidgetAction>
#include "itkTimeStamp.h"

class GlobalUIModel;
class EventBucket;
class ColorLabelQuickListWidget;

class LabelSelectionButtonRecentPanel : public QWidgetAction
{

};

class LabelSelectionButtonPopupMenu : public QMenu
{
  Q_OBJECT

public:
  LabelSelectionButtonPopupMenu(QWidget *parent);

  void SetModel(GlobalUIModel *model);

public slots:

  void onModelUpdate(const EventBucket &bucket);
  void onForegroundAction(QAction *action);
  void onBackgroundAction(QAction *action);

protected:
  void UpdateMenu();
  void UpdateCurrents();

  QMenu *m_SubForeground, *m_SubBackground;
  ColorLabelQuickListWidget *m_Recent;
  GlobalUIModel *m_Model;
  itk::TimeStamp m_LastUpdateTime;
};

/**
 * @brief The LabelSelectionButton class
 * This is an attempt to create a nice looking button with a menu that can
 * be used to select labels. The button, when pressed, shows a menu where
 * the user can quickly choose among available labels, including recently
 * used labels.
 */
class LabelSelectionButton : public QToolButton
{
  Q_OBJECT
public:
  explicit LabelSelectionButton(QWidget *parent = 0);

  void SetModel(GlobalUIModel *model);
  
signals:
  
public slots:

  void onModelUpdate(const EventBucket &bucket);
  

protected:

  GlobalUIModel *m_Model;


  void UpdateAppearance();
};

#endif // LABELSELECTIONBUTTON_H
