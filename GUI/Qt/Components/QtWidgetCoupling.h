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
#ifndef QTWIDGETCOUPLING_H
#define QTWIDGETCOUPLING_H

#include <QObject>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <EditableNumericValueModel.h>
#include <SNAPCommon.h>
#include <Property.h>
#include <LatentITKEventNotifier.h>

/**
  Abstract class for hierarchy of data mappings between widgets and models
  */
class AbstractWidgetDataMapping
{
public:
  virtual ~AbstractWidgetDataMapping() {}
  virtual void CopyFromWidgetToTarget() = 0;
  virtual void CopyFromTargetToWidget() = 0;
};


/**
  Data mapping between a numeric model and an input widget TWidget.
  The mapping handles the value of the widget, and its range.
  */
template <class TAtomic, class TWidgetPtr, class WidgetTraits>
class NumericModelWidgetDataMapping : public AbstractWidgetDataMapping
{
public:
  // The model that provides the data
  typedef AbstractEditableNumericValueModel<TAtomic> ModelType;

  // Constructor
  NumericModelWidgetDataMapping(TWidgetPtr w, ModelType *model)
    : m_Widget(w), m_Model(model), m_Updating(false) {}

  // Populate widget
  void CopyFromTargetToWidget()
  {
    m_Updating = true;

    // Prepopulate the range with current values in case the model does
    // not actually compute ranges
    NumericValueRange<TAtomic> range = WidgetTraits::GetRange(m_Widget);
    TAtomic value;

    // Obtain the value from the model
    if(m_Model->GetValueAndRange(value, &range))
      {
      WidgetTraits::SetRange(m_Widget, range);
      WidgetTraits::SetValue(m_Widget, value);
      }
    else
      {
      WidgetTraits::SetValueToNull(m_Widget);
      }

    m_Updating = false;
  }

  // Get value from widget
  void CopyFromWidgetToTarget()
  {
    if(!m_Updating)
      {
      TAtomic user_value = WidgetTraits::GetValue(m_Widget), model_value;

      // Note: if the model reports that the value is invalid, we are not
      // allowing the user to mess with the value. This may have some odd
      // consequences. We need to investigate.
      if(m_Model->GetValueAndRange(model_value, NULL) &&
         model_value != user_value)
        {
        m_Model->SetValue(user_value);
        }
      }
  }

private:

  TWidgetPtr m_Widget;
  ModelType *m_Model;
  bool m_Updating;
};












/**
  Data mapping between a property and an input widget TWidget.
  The mapping handles the value of the widget only.
  */
template <class TAtomic, class TWidget, class TPropertyContainer,
          class WidgetTraits>
class BasicPropertyToWidgetDataMapping : public AbstractWidgetDataMapping
{
public:
  typedef void (TPropertyContainer::*SetterFunction)(TAtomic);

  BasicPropertyToWidgetDataMapping(
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


/**
  Empty template for default traits. Specialize for different Qt widgets
  */
template <class TAtomic, class TWidget>
struct DefaultWidgetTraits
{
  /*
  static const char *GetSignal() { throw std::exception(); }
  static TAtomic GetValue(TWidget *) { return 0; }
  static void SetValue(TWidget *, const TAtomic &value) {}
  */
};

/**
  Default traits for the Qt Spin Box
  */
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
    w->setSpecialValueText("");
    w->setValue(static_cast<int>(value));
  }

  static void SetRange(QSpinBox *w, NumericValueRange<TAtomic> &range)
  {
    w->setMinimum(range.Minimum);
    w->setMaximum(range.Maximum);
    w->setSingleStep(range.StepSize);
  }

  static void SetValueToNull(QSpinBox *w)
  {
    w->setValue(w->minimum());
    w->setSpecialValueText(" ");
  }

  static NumericValueRange<TAtomic> GetRange(QSpinBox *w)
  {
    return NumericValueRange<TAtomic>(
          static_cast<TAtomic>(w->minimum()),
          static_cast<TAtomic>(w->maximum()),
          static_cast<TAtomic>(w->singleStep()));
  }
};

/**
  Default traits for the Qt Double Spin Box
  */
template <class TAtomic>
struct DefaultWidgetTraits<TAtomic, QDoubleSpinBox>
{
  static const char *GetSignal()
  {
    return SIGNAL(valueChanged(double));
  }

  static TAtomic GetValue(QDoubleSpinBox *w)
  {
    return static_cast<TAtomic>(w->value());
  }

  static void SetValue(QDoubleSpinBox *w, const TAtomic &value)
  {
    w->setValue(static_cast<double>(value));
    w->setSpecialValueText("");
  }

  static void SetRange(QDoubleSpinBox *w, NumericValueRange<TAtomic> &range)
  {
    w->setMinimum(range.Minimum);
    w->setMaximum(range.Maximum);
    w->setSingleStep(range.StepSize);

    // Make sure the precision is smaller than the step size. This is a
    // temporary fix. A better solution is to have the model provide the
    // precision for the widget.
    if(range.StepSize > 0)
      {
      double logstep = log10(range.StepSize);
      int prec = std::max((int) (1 - floor(logstep)), 0);
      w->setDecimals(prec);
      }
    else
      w->setDecimals(0);
  }

  static NumericValueRange<TAtomic> GetRange(QDoubleSpinBox *w)
  {
    return NumericValueRange<TAtomic>(
          static_cast<TAtomic>(w->minimum()),
          static_cast<TAtomic>(w->maximum()),
          static_cast<TAtomic>(w->singleStep()));
  }

  static void SetValueToNull(QDoubleSpinBox *w)
  {
    w->setValue(w->minimum());
    w->setSpecialValueText(" ");
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

#include <QLineEdit>

template <>
struct DefaultWidgetTraits<double, QLineEdit>
{
  static const char *GetSignal()
  {
    return SIGNAL(textEdited(const QString &));
  }

  static double GetValue(QLineEdit *w)
  {
    return atof(w->text().toAscii());
  }

  static void SetValue(QLineEdit *w, const double &value)
  {
    char buffer[32];
    sprintf(buffer, "%.4g", value);
    w->setText(buffer);
  }

  static void SetValueToNull(QLineEdit *w)
  {
    w->setText("");
  }

  static NumericValueRange<double> GetRange(QLineEdit *w)
  {
    return NumericValueRange<double>();
  }

  static void SetRange(QLineEdit *w, const NumericValueRange<double> &range)
  {
  }
};


#include <QCheckBox>
template <class TAtomic>
struct DefaultWidgetTraits<TAtomic, QCheckBox>
{
  static const char *GetSignal()
  {
    return SIGNAL(stateChanged(int));
  }

  static TAtomic GetValue(QCheckBox *w)
  {
    return static_cast<TAtomic>(w->isChecked());
  }

  static void SetValue(QCheckBox *w, const TAtomic &value)
  {
    w->setChecked(static_cast<bool>(value));
  }

  static void SetRange(QCheckBox *w, NumericValueRange<TAtomic> &range)
  {
  }

  static NumericValueRange<TAtomic> GetRange(QCheckBox *w)
  {
    return NumericValueRange<TAtomic>();
  }

  static void SetValueToNull(QCheckBox *w)
  {
    w->setChecked(false);
  }
};

#include <QRadioButton>

template <class TAtomic>
struct DefaultWidgetTraits<TAtomic, QRadioButton>
{
  static const char *GetSignal()
  {
    return SIGNAL(toggled(bool));
  }

  static TAtomic GetValue(QRadioButton *w)
  {
    return static_cast<TAtomic>(w->isChecked());
  }

  static void SetValue(QRadioButton *w, const TAtomic &value)
  {
    w->setChecked(static_cast<bool>(value));
  }

  static void SetRange(QRadioButton *w, NumericValueRange<TAtomic> &range)
  {
  }

  static NumericValueRange<TAtomic> GetRange(QRadioButton *w)
  {
    return NumericValueRange<TAtomic>();
  }

  static void SetValueToNull(QRadioButton *w)
  {
    w->setChecked(false);
  }
};

template <class TAtomic>
struct RadioButtonGroupTraits :
    public DefaultWidgetTraits<TAtomic, QWidget>
{
  static TAtomic GetValue(QWidget *w)
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

  static void SetValue(QWidget *w, const TAtomic &value)
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

  static void SetRange(QWidget *w, NumericValueRange<TAtomic> &range)
  {
  }

  static NumericValueRange<TAtomic> GetRange(QWidget *w)
  {
    return NumericValueRange<TAtomic>();
  }

  static void SetValueToNull(QWidget *w)
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
  This class allows widget traits to be extended to an array of widgets. It
  uses a child traits object to map between an iris_vector_fixed and an array
  of widgets.
  */
template <class TAtomic, unsigned int VDim, class TWidget, class WidgetTraits>
class WidgetArrayTraits
{
public:
  typedef iris_vector_fixed<TAtomic, VDim> ValueType;
  typedef std::vector<TWidget *> WidgetArrayType;

  static ValueType GetValue(WidgetArrayType wa)
  {
    ValueType value;
    for(unsigned int i = 0; i < VDim; i++)
      value(i) = WidgetTraits::GetValue(wa[i]);
    return value;
  }

  static void SetValue(WidgetArrayType wa, const ValueType &value)
  {
    for(unsigned int i = 0; i < VDim; i++)
      WidgetTraits::SetValue(wa[i], value(i));
  }

  static void SetRange(WidgetArrayType wa, NumericValueRange<ValueType> &range)
  {
    for(unsigned int i = 0; i < VDim; i++)
      {
      NumericValueRange<TAtomic> ri(
            range.Minimum(i), range.Maximum(i), range.StepSize(i));
      WidgetTraits::SetRange(wa[i], ri);
      }
  }

  static NumericValueRange<ValueType> GetRange(WidgetArrayType wa)
  {
    NumericValueRange<ValueType> range;
    for(unsigned int i = 0; i < VDim; i++)
      {
      NumericValueRange<TAtomic> ri = WidgetTraits::GetRange(wa[i]);
      range.Minimum(i) = ri.Minimum;
      range.Maximum(i) = ri.Maximum;
      range.StepSize(i) = ri.StepSize;
      }
    return range;
  }

  static void SetValueToNull(WidgetArrayType wa)
  {
    for(unsigned int i = 0; i < VDim; i++)
      WidgetTraits::SetValueToNull(wa[i]);
  }

  static const char *GetSignal()
  {
    return WidgetTraits::GetSignal();
  }
};


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

  typedef BasicPropertyToWidgetDataMapping<
      TAtomic, TWidget, TContainer, WidgetTraits> MappingType;

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


template <class TAtomic, class TWidget, class WidgetTraits>
void makeCoupling(TWidget *w,
                  AbstractEditableNumericValueModel<TAtomic> *model)
{
  typedef NumericModelWidgetDataMapping<
      TAtomic, TWidget *, WidgetTraits> MappingType;
  MappingType *mapping = new MappingType(w, model);
  QtCouplingHelper *h = new QtCouplingHelper(w, mapping);

  // Populate the widget
  mapping->CopyFromTargetToWidget();

  // Listen to value change events from the model
  LatentITKEventNotifier::connect(
        model, ValueChangedEvent(),
        h, SLOT(onPropertyModification()));

  LatentITKEventNotifier::connect(
        model, RangeChangedEvent(),
        h, SLOT(onPropertyModification()));

  // Listen to value change events for this widget
  h->connect(w, WidgetTraits::GetSignal(), SLOT(onUserModification()));
}

/** Create a coupling between a numeric model and a Qt widget. The widget
  will listen to the events from the model and update its value and range
  accordingly. When the user interacts with the widget, the model will be
  updated. The coupling fully automates mapping of data between Qt input
  widgets and SNAP numeric models.

  This version of the method uses default traits. There is also a version
  that allows you to provide your own traits.
*/
template <class TAtomic, class TWidget>
void makeCoupling(TWidget *w,
                  AbstractEditableNumericValueModel<TAtomic> *model)
{
  typedef DefaultWidgetTraits<TAtomic, TWidget> WidgetTraits;
  makeCoupling<TAtomic, TWidget, WidgetTraits>(w, model);
}


/**
  Create a coupling between an model whose value is of a vector type and
  an array of widgets of the same type. For example, this function allows
  you to hook up a model wrapped around a Vector3d to a triple of spin boxes.
  This is very convenient for dealing with input and output of vector data.
  */
template <unsigned int VDim, class TAtomic, class TWidget, class WidgetTraits>
void makeArrayCoupling(std::vector<TWidget *> wa,
                       AbstractEditableNumericValueModel<
                           iris_vector_fixed<TAtomic, VDim> > *model)
{
  typedef std::vector<TWidget *> WidgetArray;
  typedef WidgetArrayTraits<TAtomic, VDim, TWidget, WidgetTraits> ArrayTraits;
  typedef iris_vector_fixed<TAtomic, VDim> ValueType;
  typedef NumericModelWidgetDataMapping<
      ValueType, WidgetArray, ArrayTraits> MappingType;

  // Create the mapping
  MappingType *mapping = new MappingType(wa, model);

  // Create the coupling helper (event handler). It's attached to the first
  // widget, just for the purpose of this object being deleted later.
  QtCouplingHelper *h = new QtCouplingHelper(wa.front(), mapping);

  // Listen to value change events from the model
  LatentITKEventNotifier::connect(
        model, ValueChangedEvent(),
        h, SLOT(onPropertyModification()));

  LatentITKEventNotifier::connect(
        model, RangeChangedEvent(),
        h, SLOT(onPropertyModification()));

  // Listen to value change events for this widget
  for(unsigned int i = 0; i < VDim; i++)
    h->connect(wa[i], WidgetTraits::GetSignal(), SLOT(onUserModification()));
}

template <class TAtomic, class TWidget, class WidgetTraits>
void makeArrayCoupling(TWidget *w1, TWidget *w2, TWidget *w3,
                       AbstractEditableNumericValueModel<
                           iris_vector_fixed<TAtomic, 3> > *model)
{
  // Create the array of widgets
  std::vector<TWidget *> wa(3);
  wa[0] = w1; wa[1] = w2; wa[2] = w3;

  // Call the main method
  makeArrayCoupling<3, TAtomic, TWidget, WidgetTraits>(wa, model);
}

template <class TAtomic, class TWidget>
void makeArrayCoupling(TWidget *w1, TWidget *w2, TWidget *w3,
                       AbstractEditableNumericValueModel<
                           iris_vector_fixed<TAtomic, 3> > *model)
{
  // Call the main method
  typedef DefaultWidgetTraits<TAtomic, TWidget> WidgetTraits;
  makeArrayCoupling<TAtomic, TWidget, WidgetTraits>(w1, w2, w3, model);
}



/**
  Create a coupling between a widget containing a set of radio buttons
  and an enum. The values of the enum must be 0,1,2,... The buttons must
  be in the same order as the enum entries.
  */
template <class TAtomic, class TWidget>
void makeRadioGroupCoupling(
    TWidget *w, AbstractEditableNumericValueModel<TAtomic> *model)
{
  typedef RadioButtonGroupTraits<TAtomic> WidgetTraits;
  typedef NumericModelWidgetDataMapping<
      TAtomic, TWidget *, WidgetTraits> MappingType;
  MappingType *mapping = new MappingType(w, model);
  QtCouplingHelper *h = new QtCouplingHelper(w, mapping);

  // Populate the widget
  mapping->CopyFromTargetToWidget();

  // Listen to value change events from the model
  LatentITKEventNotifier::connect(
        model, ValueChangedEvent(),
        h, SLOT(onPropertyModification()));

  LatentITKEventNotifier::connect(
        model, RangeChangedEvent(),
        h, SLOT(onPropertyModification()));

  // Listen to value change events for every child widget
  const QObjectList &kids = w->children();
  for(QObjectList::const_iterator it = kids.begin(); it != kids.end(); ++it)
    {
    if((*it)->inherits("QAbstractButton"))
      {
      QAbstractButton *qab = static_cast<QAbstractButton *>(*it);
      h->connect(qab, SIGNAL(toggled(bool)), SLOT(onUserModification()));
      }
    }
}


#endif // QTWIDGETCOUPLING_H
