#ifndef QTACTIONGROUPCOUPLING_H
#define QTACTIONGROUPCOUPLING_H

#include <QtCheckableWidgetGroupCoupling.h>
#include <QAction>
#include <QActionGroup>

/**
 * Create a coupling between an enum (not necessarily starting with zero) and an
 * QActionGroup containing a set of actions. The mapping from enum values to
 * actions is provided by the actionMap parameter
 */
template <class TAtomic>
void makeActionGroupCoupling(
    QActionGroup *actionGroup,
    std::map<TAtomic, QAction *> actionMap,
    AbstractPropertyModel<TAtomic> *model)
{
  makeCheckableWidgetGroupCoupling(actionGroup, actionMap, model);
}


/**
 * Create a coupling between an enum (starting with zero) and an QActionGroup
 */
template <class TAtomic>
void makeActionGroupCoupling(
    QActionGroup *w, AbstractPropertyModel<TAtomic> *model)
{
  QList<QAction *> kids = w->actions();
  std::map<TAtomic, QAction *> buttonMap;
  int iwidget = 0;
  for(QList<QAction *>::const_iterator it = kids.begin(); it != kids.end(); ++it)
    {
    QAction *qab = dynamic_cast<QAction *>(*it);
    if(qab)
      buttonMap[static_cast<TAtomic>(iwidget++)] = qab;
    }
  makeCheckableWidgetGroupCoupling(w, buttonMap, model);
}








#endif // QTACTIONGROUPCOUPLING_H
