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

template <class TAtomic>
struct RadioButtonGroupTraits :
    public WidgetValueTraitsBase<TAtomic, QWidget *>
{
public:
  TAtomic GetValue(QWidget *w)
  {
    // Figure out which button is checked
    int ifound = 0;
    for(QObjectList::const_iterator it = w->children().begin();
        it != w->children().end(); ++it)
      {
      if((*it)->inherits("QAbstractButton"))
        {
        QAbstractButton *qab = static_cast<QAbstractButton *>(*it);
        if(qab->isChecked())
          break;
        ++ifound;
        }
      }
    return static_cast<TAtomic>(ifound);
  }

  void SetValue(QWidget *w, const TAtomic &value)
  {
    // Figure out which button is checked
    int ifound = (int) value;

    // Set all the radios
    for(QObjectList::const_iterator it = w->children().begin();
        it != w->children().end(); ++it)
      {
      if((*it)->inherits("QAbstractButton"))
        {
        QAbstractButton *qab = static_cast<QAbstractButton *>(*it);
        qab->setChecked(ifound-- == 0);
        }
      }
  }

  void SetValueToNull(QWidget *w)
  {
    // Set all the radios
    for(QObjectList::const_iterator it = w->children().begin();
        it != w->children().end(); ++it)
      {
      if((*it)->inherits("QAbstractButton"))
        {
        QAbstractButton *qab = static_cast<QAbstractButton *>(*it);
        qab->setChecked(false);
        }
      }
  }
};


/**
  Create a coupling between a widget containing a set of radio buttons
  and an enum. The values of the enum must be 0,1,2,... The order of the
  buttons in the last parameter should match the order of the enum. In case
  that the order of the radio buttons in the parent widget already matches
  the order of the enum entries, call the second version of this method, below
  */
template <class TAtomic, class TWidget>
void makeRadioGroupCoupling(
    TWidget *w, AbstractPropertyModel<TAtomic> *model,
    const QObjectList &wlist)
{
  typedef AbstractPropertyModel<TAtomic> ModelType;
  typedef RadioButtonGroupTraits<TAtomic> WidgetValueTraits;
  typedef DefaultWidgetDomainTraits<TrivialDomain, TWidget> WidgetDomainTraits;

  typedef PropertyModelToWidgetDataMapping<
      ModelType, TWidget *,
      WidgetValueTraits, WidgetDomainTraits> MappingType;

  WidgetValueTraits valueTraits;
  WidgetDomainTraits domainTraits;
  MappingType *mapping = new MappingType(w, model, valueTraits, domainTraits);
  QtCouplingHelper *h = new QtCouplingHelper(w, mapping);

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

  for(QObjectList::const_iterator it = wlist.begin(); it != wlist.end(); ++it)
    {
    if((*it)->inherits("QAbstractButton"))
      {
      QAbstractButton *qab = static_cast<QAbstractButton *>(*it);
      h->connect(qab, SIGNAL(toggled(bool)), SLOT(onUserModification()));
      }
    }
}

/**
  Create a coupling between a widget containing a set of radio buttons
  and an enum. The values of the enum must be 0,1,2,... The order of the
  radio button widgets in the parent widget *w should match the order of
  the enum entries. If not, see the method above.
  */
template <class TAtomic, class TWidget>
void makeRadioGroupCoupling(
    TWidget *w, AbstractPropertyModel<TAtomic> *model)
{
  const QObjectList &kids = w->children();
  makeRadioGroupCoupling(w, model, kids);
}

#endif // QTRADIOBUTTONCOUPLING_H
