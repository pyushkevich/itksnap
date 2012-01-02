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

#ifndef QTDOUBLESPINBOXCOUPLING_H
#define QTDOUBLESPINBOXCOUPLING_H

#include <QObject>
#include <EditableNumericValueModel.h>
#include "SNAPCommon.h"

class EventBucket;
class QDoubleSpinBox;

class QtDoubleSpinboxCoupling : public QObject
{
  Q_OBJECT

public:

  // The model that couples with this object
  typedef AbstractEditableNumericValueModel<double> ModelType;

  explicit QtDoubleSpinboxCoupling(QDoubleSpinBox *widget, ModelType *model);

  virtual ~QtDoubleSpinboxCoupling();

public slots:

  void onModelUpdate(const EventBucket &);
  void onWidgetValueChanged(double value);

protected:

  void updateWidgetFromModel();

  QDoubleSpinBox *m_Widget;
  ModelType *m_Model;
  bool m_Updating;

  int m_DefaultDecimals;
};



class QSpinBox;
#include <Property.h>

class AbstractWidgetDataMapping
{
public:
  virtual ~AbstractWidgetDataMapping() {}
  virtual void CopyFromWidgetToTarget() = 0;
  virtual void CopyFromTargetToWidget() = 0;
};

template <class TAtomic, class TWidget, class TPropertyContainer,
          class WidgetTraits>
class BasicWidgetDataMapping : public AbstractWidgetDataMapping
{
public:
  typedef void (TPropertyContainer::*SetterFunction)(TAtomic);

  BasicWidgetDataMapping(
      TWidget *w, ConstProperty<TAtomic> &p,
      SetterFunction setter, TPropertyContainer *c)
    : m_Widget(w), m_Property(p), m_Setter(setter), m_Container(c) {}

  void CopyFromWidgetToTarget()
  {
    TAtomic x = WidgetTraits::GetValue(m_Widget);
    if(x != m_Property)
      ((*m_Container).*(m_Setter))(x);
  }

  void CopyFromTargetToWidget()
  {
    TAtomic x = m_Property;
    if(x != WidgetTraits::GetValue(m_Widget))
      WidgetTraits::SetValue(m_Widget, x);
  }

protected:
  TWidget *m_Widget;
  ConstProperty<TAtomic> &m_Property;
  SetterFunction m_Setter;
  TPropertyContainer *m_Container;
};


template <class TAtomic, class TWidget>
struct DefaultWidgetTraits
{
  /*
  static const char *GetSignal() { throw std::exception(); }
  static TAtomic GetValue(TWidget *) { return 0; }
  static void SetValue(TWidget *, const TAtomic &value) {}
  */
};


#include <QSpinBox>
#include <QSlider>

template <class TAtomic>
struct DefaultWidgetTraits<TAtomic, QSpinBox>
{
  static const char *GetSignal()
  {
    return SIGNAL(valueChanged(int));
  }

  static TAtomic GetValue(QSpinBox *w)
  {
    return static_cast<TAtomic>(w->value());
  }

  static void SetValue(QSpinBox *w, const TAtomic &value)
  {
    w->setValue(static_cast<int>(value));
  }
};

template <class TAtomic>
struct DefaultWidgetTraits<TAtomic, QSlider>
{
  static const char *GetSignal()
  {
    return SIGNAL(valueChanged(int));
  }

  static TAtomic GetValue(QAbstractSlider *w)
  {
    return static_cast<TAtomic>(w->value());
  }

  static void SetValue(QAbstractSlider *w, const TAtomic &value)
  {
    w->setValue(static_cast<int>(value));
  }
};

#include "LatentITKEventNotifier.h"

class QtCouplingHelper : public QObject
{
  Q_OBJECT

public:
  explicit QtCouplingHelper(QWidget *widget, AbstractWidgetDataMapping *dm)
    : QObject(widget)
  {
    m_DataMapping = dm;
  }


public slots:
  void onUserModification()
  {
    m_DataMapping->CopyFromWidgetToTarget();
  }

  void onPropertyModification()
  {
    m_DataMapping->CopyFromTargetToWidget();
  }


protected:
  AbstractWidgetDataMapping *m_DataMapping;
};

template <class TAtomic, class TWidget, class TContainer, class WidgetTraits>
void makeCoupling(TWidget *w, TContainer *c,
                  void (TContainer::*setter)(TAtomic),
                  ConstProperty<TAtomic> & (TContainer::*getter)())
{
  // Retrieve the property
  ConstProperty<TAtomic> &p = ((*c).*(getter))();

  typedef BasicWidgetDataMapping<TAtomic, TWidget, TContainer, WidgetTraits>
      MappingType;

  MappingType *mapping = new MappingType(w, p, setter, c);

  QtCouplingHelper *h = new QtCouplingHelper(w, mapping);

  // The property should notify the widget of changes
  LatentITKEventNotifier::connect(
        &p, PropertyChangeEvent(),
        h, SLOT(onPropertyModification()));

  // Coupling helper listens to events from widget
  h->connect(w, WidgetTraits::GetSignal(), SLOT(onUserModification()));
}

template <class TAtomic, class TWidget, class TContainer>
void makeCoupling(TWidget *w, TContainer *c,
                  void (TContainer::*setter)(TAtomic),
                  ConstProperty<TAtomic> & (TContainer::*getter)())
{
  typedef DefaultWidgetTraits<TAtomic, TWidget> WidgetTraits;
  makeCoupling<TAtomic, TWidget, TContainer, WidgetTraits>(w,c,setter,getter);
}


#endif // QTDOUBLESPINBOXCOUPLING_H
