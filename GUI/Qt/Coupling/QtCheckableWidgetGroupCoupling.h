#ifndef QTCHECKABLEWIDGETGROUPCOUPLING_H
#define QTCHECKABLEWIDGETGROUPCOUPLING_H

#include <QtWidgetCoupling.h>

template <class TAtomic, class TParentWidget, class TCheckableWidgetBase>
struct RadioButtonGroupTraits :
    public WidgetValueTraitsBase<TAtomic, TParentWidget *>
{
public:
  typedef std::map<TAtomic, TCheckableWidgetBase *> ButtonMap;
  typedef typename ButtonMap::iterator ButtonIterator;

  RadioButtonGroupTraits(ButtonMap bm) : m_ButtonMap(bm) {}

  TAtomic GetValue(TParentWidget *w)
  {
    // Figure out which button is checked
    for(ButtonIterator it = m_ButtonMap.begin(); it != m_ButtonMap.end(); ++it)
      {
        TCheckableWidgetBase *qab = it->second;
        if(qab->isChecked())
          return it->first;
      }

    // This is ambiguous...
    return static_cast<TAtomic>(0);
  }

  void SetValue(TParentWidget *w, const TAtomic &value)
  {
    // Set all the buttons
    for(ButtonIterator it = m_ButtonMap.begin(); it != m_ButtonMap.end(); ++it)
      {
      TCheckableWidgetBase *qab = it->second;
      qab->setChecked(it->first == value);
      }
  }

  void SetValueToNull(TParentWidget *w)
  {
    // Set all the buttons
    for(ButtonIterator it = m_ButtonMap.begin(); it != m_ButtonMap.end(); ++it)
      {
      TCheckableWidgetBase *qab = it->second;
      qab->setChecked(false);
      }
  }

protected:

  ButtonMap m_ButtonMap;
};


template <class TAtomic, class TParentWidget, class TCheckableWidgetBase>
void makeCheckableWidgetGroupCoupling(
    TParentWidget *parentWidget,
    std::map<TAtomic, TCheckableWidgetBase *> buttonMap,
    AbstractPropertyModel<TAtomic> *model)
{
  typedef AbstractPropertyModel<TAtomic> ModelType;
  typedef RadioButtonGroupTraits<TAtomic, TParentWidget, TCheckableWidgetBase> WidgetValueTraits;
  typedef DefaultWidgetDomainTraits<TrivialDomain, TParentWidget> WidgetDomainTraits;

  typedef PropertyModelToWidgetDataMapping<
      ModelType, TParentWidget *,
      WidgetValueTraits, WidgetDomainTraits> MappingType;

  WidgetValueTraits valueTraits(buttonMap);
  WidgetDomainTraits domainTraits;
  MappingType *mapping = new MappingType(parentWidget, model, valueTraits, domainTraits);
  QtCouplingHelper *h = new QtCouplingHelper(parentWidget, mapping);

  // Populate the widget
  mapping->InitializeWidgetFromModel();

  // Listen to value change events from the model
  LatentITKEventNotifier::connect(
        model, ValueChangedEvent(),
        h, SLOT(onPropertyModification(const EventBucket &)));

  LatentITKEventNotifier::connect(
        model, DomainChangedEvent(),
        h, SLOT(onPropertyModification(const EventBucket &)));

  // Listen to value change events for every child widget
  typedef typename std::map<TAtomic, TCheckableWidgetBase *>::const_iterator Iter;
  for(Iter it = buttonMap.begin(); it != buttonMap.end(); ++it)
    {
      TCheckableWidgetBase *qab = it->second;
      h->connect(qab, SIGNAL(toggled(bool)), SLOT(onUserModification()));
    }
}


#endif // QTCHECKABLEWIDGETGROUPCOUPLING_H
