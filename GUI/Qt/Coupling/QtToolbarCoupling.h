#ifndef QTTOOLBARCOUPLING_H
#define QTTOOLBARCOUPLING_H

#include "QtWidgetCoupling.h"
#include "QToolBar"
#include "QAbstractButton"
#include <ColorLabel.h>
#include <SNAPQtCommon.h>


/**
 * This class defines a coupling between a QToolbar in which there are a
 * number of mutually exclusive actions, which are assigned a value of the
 * TAtomic type using the 'data' parameter (using QAction::setData).
 */
template <class TAtomic>
class DefaultWidgetValueTraits<TAtomic, QToolBar>
    : public WidgetValueTraitsBase<TAtomic, QToolBar *>
{
public:

  // Luckily a QToolBar provides a signal for us to use
  virtual const char *GetSignal()
  {
    return SIGNAL(actionTriggered(QAction *));
  }

  // We must trigger the action that corresponds to the value. For this
  // to work, we must set the user data of the action to the value
  virtual void SetValue(QToolBar *w, const TAtomic &value)
  {
    foreach(QAction *action, w->actions())
      {
      if(value == action->data().value<TAtomic>())
        action->setChecked(true);
      else
        action->setChecked(false);
      }
  }

  // Get the value - i.e., the action that is currently checked
  virtual TAtomic GetValue(QToolBar *w)
  {
    foreach(QAction *action, w->actions())
      if(action->isChecked())
        return action->data().value<TAtomic>();
    return 0;
  }

  // Setting value to NULL means unchecking all actions
  virtual void SetValueToNull(QToolBar *w)
  {
    foreach(QAction *action, w->actions())
      {
      action->setChecked(false);
      }
  }
};



/**
 * This class populates an action group to match a domain that is a set of
 * items. The actions in the action group are dynamically allocated
 */
/*
template <class TItemDomain, class TRowTraits>
class DefaultMultiRowWidgetDomainTraits<TItemDomain, QToolBar, TRowTraits> :
    public WidgetDomainTraitsBase<TItemDomain, QToolBar *>
{
  // The information about the item type are taken from the domain
  typedef typename TItemDomain::ValueType AtomicType;
  typedef typename TItemDomain::DescriptorType DescriptorType;
  typedef TItemDomain DomainType;

  void SetDomain(QToolBar *w, const DomainType &domain)
  {
    // Remove all the actions from the toolbar
    w->clear();

    // Create an action group unless one already exists in the widget
    QActionGroup *ag = w->findChild<QActionGroup *>();
    if(ag == NULL)
      ag = new QActionGroup(w);

    // Populate the toolbar with actions
    for(typename DomainType::const_iterator it = domain.begin();
        it != domain.end(); ++it)
      {
      // Get the key/value pair
      AtomicType value = domain.GetValue(it);
      const DescriptorType &row = domain.GetDescription(it);

      // Create an action and assign it to the action group
      QAction *action = new QAction(w);
      action->setActionGroup(ag);
      action->setCheckable(true);
      action->setData(QVariant::fromValue(value));

      // Use the row traits to map information to the widget
      TRowTraits::updateAction(action, row);
      }
  }

  void UpdateDomainDescription(QToolBar *w, const DomainType &domain)
  {
    // Update each of the actions
    foreach(QAction *action, w->actions())
      {
      AtomicType value = action->data().value<AtomicType>();
      typename DomainType::const_iterator it = domain.find(value);
      if(it != domain.end())
        {
        TRowTraits::updateAction(action, domain.GetDescription(it));
        }
      }
  }

  TItemDomain GetDomain(QToolBar *w)
  {
    // We don't actually pull the widget because the domain is fully specified
    // by the model.
    return DomainType();
  }

};
*/

/**
 * A row traits that populates a QAction from a color label
 *
 * Use in conjunction with makeMultiRowCoupling
 */
/*
class ColorLabelToQActionRowTraits
{
public:
  static void updateAction(QAction *action, const ColorLabel &label)
  {
    action->setIcon(CreateColorBoxIcon(16, 16, GetBrushForColorLabel(label)));
    action->setToolTip(QString::fromUtf8(label.GetLabel()));
  }
};
*/

#endif // QTTOOLBARCOUPLING_H
