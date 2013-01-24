/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: Filename.cxx,v $
  Language:  C++
  Date:      $Date: 2010/10/18 11:25:44 $
  Version:   $Revision: 1.12 $
  Copyright (c) 2011 Paul A. Yushkevich

  This file is part of ITK-SNAP

  ITK-SNAP is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

=========================================================================*/

#ifndef QTRADIOBUTTONCOUPLING_H
#define QTRADIOBUTTONCOUPLING_H

#include <QtWidgetCoupling.h>
#include <QRadioButton>
#include <map>

template <class TAtomic>
struct RadioButtonGroupTraits :
    public WidgetValueTraitsBase<TAtomic, QWidget *>
{
public:
  typedef std::map<TAtomic, QAbstractButton *> ButtonMap;
  typedef typename ButtonMap::iterator ButtonIterator;

  RadioButtonGroupTraits(ButtonMap bm) : m_ButtonMap(bm) {}

  TAtomic GetValue(QWidget *w)
  {
    // Figure out which button is checked
    for(ButtonIterator it = m_ButtonMap.begin(); it != m_ButtonMap.end(); ++it)
      {
        QAbstractButton *qab = it->second;
        if(qab->isChecked())
          return it->first;
      }

    // This is ambiguous...
    return static_cast<TAtomic>(0);
  }

  void SetValue(QWidget *w, const TAtomic &value)
  {
    // Set all the buttons
    for(ButtonIterator it = m_ButtonMap.begin(); it != m_ButtonMap.end(); ++it)
      {
      QAbstractButton *qab = it->second;
      qab->setChecked(it->first == value);
      }
  }

  void SetValueToNull(QWidget *w)
  {
    // Set all the buttons
    for(ButtonIterator it = m_ButtonMap.begin(); it != m_ButtonMap.end(); ++it)
      {
      QAbstractButton *qab = it->second;
      qab->setChecked(false);
      }
  }

protected:

  ButtonMap m_ButtonMap;
};


/**
  Create a coupling between a widget containing a set of radio buttons
  and a set of values of type TAtomic (true/false, enum, integer, etc).
  The mapping of values to button widgets is provided in the third parameter.
  */
template <class TAtomic>
void makeRadioGroupCoupling(
    QWidget *parentWidget,
    std::map<TAtomic, QAbstractButton *> buttonMap,
    AbstractPropertyModel<TAtomic> *model)
{
  typedef AbstractPropertyModel<TAtomic> ModelType;
  typedef RadioButtonGroupTraits<TAtomic> WidgetValueTraits;
  typedef DefaultWidgetDomainTraits<TrivialDomain, QWidget> WidgetDomainTraits;

  typedef PropertyModelToWidgetDataMapping<
      ModelType, QWidget *,
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
  typedef typename std::map<TAtomic, QAbstractButton *>::const_iterator Iter;
  for(Iter it = buttonMap.begin(); it != buttonMap.end(); ++it)
    {
      QAbstractButton *qab = it->second;
      h->connect(qab, SIGNAL(toggled(bool)), SLOT(onUserModification()));
    }
}


/**
  Create a coupling between a widget containing a set of radio buttons
  and an enum. The values of the enum must be 0,1,2,... The order of the
  radio button widgets in the parent widget *w should match the order of
  the enum entries. This method only fits specific situations, in other
  cases see the more general version above
  */
template <class TAtomic>
void makeRadioGroupCoupling(
    QWidget *w, AbstractPropertyModel<TAtomic> *model)
{
  const QObjectList &kids = w->children();
  std::map<TAtomic, QAbstractButton *> buttonMap;
  int iwidget = 0;
  for(QObjectList::const_iterator it = kids.begin(); it != kids.end(); ++it)
    {
    QAbstractButton *qab = dynamic_cast<QAbstractButton *>(*it);
    if(qab)
      buttonMap[static_cast<TAtomic>(iwidget++)] = qab;
    }
  makeRadioGroupCoupling(w, buttonMap, model);
}

#endif // QTRADIOBUTTONCOUPLING_H
