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
  NumericModelWidgetDataMapping(TWidgetPtr w, ModelType *model,
                                WidgetTraits traits)
    : m_Widget(w), m_Model(model), m_Updating(false), m_Traits(traits) {}

  // Populate widget
  void CopyFromTargetToWidget()
  {
    m_Updating = true;

    // Prepopulate the range with current values in case the model does
    // not actually compute ranges
    NumericValueRange<TAtomic> range = m_Traits.GetRange(m_Widget);
    TAtomic value;

    // Obtain the value from the model
    if(m_Model->GetValueAndRange(value, &range))
      {
      m_Traits.SetRange(m_Widget, range);
      m_Traits.SetValue(m_Widget, value);
      }
    else
      {
      m_Traits.SetValueToNull(m_Widget);
      }

    m_Updating = false;
  }

  // Get value from widget
  void CopyFromWidgetToTarget()
  {
    if(!m_Updating)
      {
      TAtomic user_value = m_Traits.GetValue(m_Widget);
      TAtomic model_value;

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
  WidgetTraits m_Traits;
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
      SetterFunction setter, TPropertyContainer *c,
      WidgetTraits traits)
    : m_Widget(w), m_Property(p), m_Setter(setter),
      m_Container(c), m_Traits(traits) {}

  void CopyFromWidgetToTarget()
  {
    TAtomic x = m_Traits.GetValue(m_Widget);
    if(x != m_Property)
      ((*m_Container).*(m_Setter))(x);
  }

  void CopyFromTargetToWidget()
  {
    TAtomic x = m_Property;
    if(x != m_Traits.GetValue(m_Widget))
      m_Traits.SetValue(m_Widget, x);
  }

protected:
  TWidget *m_Widget;
  ConstProperty<TAtomic> &m_Property;
  SetterFunction m_Setter;
  TPropertyContainer *m_Container;
  WidgetTraits m_Traits;
};


/**
  Base class for widget traits
  */
template <class TAtomic, class TWidgetPtr>
class WidgetTraitsBase
{
public:
  // Get the Qt signal that the widget fires when its value has changed
  virtual const char *GetSignal() { return NULL; }

  // Get the value from the widget
  virtual TAtomic GetValue(TWidgetPtr) = 0;

  // Set the value of the widget
  virtual void SetValue(TWidgetPtr, const TAtomic &) = 0;

  // The default behavior is to ignore the range. Child classes where the
  // range is of relevance should override these functions
  virtual void SetRange(TWidgetPtr w, NumericValueRange<TAtomic> &range) {}
  virtual NumericValueRange<TAtomic> GetRange(TWidgetPtr w)
    { return NumericValueRange<TAtomic> (); }

  // The default behavior for setting the widget to null is to do nothing.
  // This should be overridden by child traits classes
  virtual void SetValueToNull(TWidgetPtr) {};
};

/**
  Empty template for default traits. Specialize for different Qt widgets
  */

template <class TAtomic, class TWidget>
class DefaultWidgetTraits
{

};


/**
  Default traits for the Qt Spin Box
  */
template <class TAtomic>
class DefaultWidgetTraits<TAtomic, QSpinBox>
    : public WidgetTraitsBase<TAtomic, QSpinBox *>
{
public:
  const char *GetSignal()
  {
    return SIGNAL(valueChanged(int));
  }

  TAtomic GetValue(QSpinBox *w)
  {
    return static_cast<TAtomic>(w->value());
  }

  void SetValue(QSpinBox *w, const TAtomic &value)
  {
    w->setSpecialValueText("");
    w->setValue(static_cast<int>(value));
  }

  void SetRange(QSpinBox *w, NumericValueRange<TAtomic> &range)
  {
    w->setMinimum(range.Minimum);
    w->setMaximum(range.Maximum);
    w->setSingleStep(range.StepSize);
  }

  void SetValueToNull(QSpinBox *w)
  {
    w->setValue(w->minimum());
    w->setSpecialValueText(" ");
  }

  NumericValueRange<TAtomic> GetRange(QSpinBox *w)
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
class DefaultWidgetTraits<TAtomic, QDoubleSpinBox>
    : public WidgetTraitsBase<TAtomic, QDoubleSpinBox *>
{
public:
  const char *GetSignal()
  {
    return SIGNAL(valueChanged(double));
  }

  TAtomic GetValue(QDoubleSpinBox *w)
  {
    return static_cast<TAtomic>(w->value());
  }

  void SetValue(QDoubleSpinBox *w, const TAtomic &value)
  {
    w->setSpecialValueText("");
    w->setValue(static_cast<double>(value));
  }

  void SetRange(QDoubleSpinBox *w, NumericValueRange<TAtomic> &range)
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

  NumericValueRange<TAtomic> GetRange(QDoubleSpinBox *w)
  {
    return NumericValueRange<TAtomic>(
          static_cast<TAtomic>(w->minimum()),
          static_cast<TAtomic>(w->maximum()),
          static_cast<TAtomic>(w->singleStep()));
  }

  void SetValueToNull(QDoubleSpinBox *w)
  {
    w->setValue(w->minimum());
    w->setSpecialValueText(" ");
  }
};

template <class TAtomic>
class DefaultWidgetTraits<TAtomic, QSlider>
    : public WidgetTraitsBase<TAtomic, QSlider *>
{
public:
  const char *GetSignal()
  {
    return SIGNAL(valueChanged(int));
  }

  TAtomic GetValue(QSlider *w)
  {
    return static_cast<TAtomic>(w->value());
  }

  void SetValue(QSlider *w, const TAtomic &value)
  {
    w->setValue(static_cast<int>(value));
  }
};

#include <QLineEdit>
#include <iostream>
#include <iomanip>


template <class TAtomic>
class DefaultWidgetTraits<TAtomic, QLineEdit>
    : public WidgetTraitsBase<TAtomic, QLineEdit *>
{
public:

  virtual TAtomic GetValue(QLineEdit *w)
  {
    std::istringstream iss(w->text().toStdString());
    TAtomic value;
    iss >> value;
    return value;
  }

  virtual void SetValue(QLineEdit *w, const TAtomic &value)
  {
    std::ostringstream oss;
    oss << value;
    w->setText(oss.str().c_str());
  }

  virtual void SetValueToNull(QLineEdit *w)
  {
    w->setText("");
  }

  virtual const char *GetSignal()
  {
    return SIGNAL(textEdited(const QString &));
  }
};



/** Base class for traits that map between a numeric value and a text editor */
template <class TAtomic, class QLineEdit>
class FixedPrecisionRealToTextFieldWidgetTraits
    : public DefaultWidgetTraits<TAtomic, QLineEdit>
{
public:

  FixedPrecisionRealToTextFieldWidgetTraits(int precision)
    : m_Precision(precision) {}

  irisGetSetMacro(Precision, int)

  virtual void SetValue(QLineEdit *w, const TAtomic &value)
  {
    std::ostringstream oss;
    oss << std::setprecision(m_Precision) << value;
    w->setText(oss.str().c_str());
  }

protected:

  int m_Precision;
};


#include <QCheckBox>
template <class TAtomic>
struct DefaultWidgetTraits<TAtomic, QCheckBox>
    : public WidgetTraitsBase<TAtomic, QCheckBox *>
{
public:
  const char *GetSignal()
  {
    return SIGNAL(stateChanged(int));
  }

  TAtomic GetValue(QCheckBox *w)
  {
    return static_cast<TAtomic>(w->isChecked());
  }

  void SetValue(QCheckBox *w, const TAtomic &value)
  {
    w->setChecked(static_cast<bool>(value));
  }

  void SetValueToNull(QCheckBox *w)
  {
    w->setChecked(false);
  }
};

#include <QRadioButton>

template <class TAtomic>
struct DefaultWidgetTraits<TAtomic, QRadioButton>
{
public:
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

  static void SetValueToNull(QRadioButton *w)
  {
    w->setChecked(false);
  }
};

template <class TAtomic>
struct RadioButtonGroupTraits :
    public WidgetTraitsBase<TAtomic, QWidget *>
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
  This class allows widget traits to be extended to an array of widgets. It
  uses a child traits object to map between an iris_vector_fixed and an array
  of widgets.
  */
template <class TAtomic, unsigned int VDim, class TWidget, class WidgetTraits>
class WidgetArrayTraits :
    public WidgetTraitsBase< iris_vector_fixed<TAtomic, VDim>,
                             std::vector<TWidget *> >
{
public:
  typedef iris_vector_fixed<TAtomic, VDim> ValueType;
  typedef std::vector<TWidget *> WidgetArrayType;

  /**
    Constructor, takes the "child" traits object, i.e., the traits for the
    individual widgets in the array
    */
  WidgetArrayTraits(WidgetTraits childTraits)
    : m_ChildTraits(childTraits) {}

  ValueType GetValue(WidgetArrayType wa)
  {
    ValueType value;
    for(unsigned int i = 0; i < VDim; i++)
      value(i) = m_ChildTraits.GetValue(wa[i]);
    return value;
  }

  void SetValue(WidgetArrayType wa, const ValueType &value)
  {
    for(unsigned int i = 0; i < VDim; i++)
      m_ChildTraits.SetValue(wa[i], value(i));
  }

  void SetRange(WidgetArrayType wa, NumericValueRange<ValueType> &range)
  {
    for(unsigned int i = 0; i < VDim; i++)
      {
      NumericValueRange<TAtomic> ri(
            range.Minimum(i), range.Maximum(i), range.StepSize(i));
      m_ChildTraits.SetRange(wa[i], ri);
      }
  }

  NumericValueRange<ValueType> GetRange(WidgetArrayType wa)
  {
    NumericValueRange<ValueType> range;
    for(unsigned int i = 0; i < VDim; i++)
      {
      NumericValueRange<TAtomic> ri = m_ChildTraits.GetRange(wa[i]);
      range.Minimum(i) = ri.Minimum;
      range.Maximum(i) = ri.Maximum;
      range.StepSize(i) = ri.StepSize;
      }
    return range;
  }

  void SetValueToNull(WidgetArrayType wa)
  {
    for(unsigned int i = 0; i < VDim; i++)
      m_ChildTraits.SetValueToNull(wa[i]);
  }

  const char *GetSignal()
  {
    return m_ChildTraits.GetSignal();
  }

protected:
  WidgetTraits m_ChildTraits;
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
                  ConstProperty<TAtomic> & (TContainer::*getter)(),
                  WidgetTraits traits)
{
  // Retrieve the property
  ConstProperty<TAtomic> &p = ((*c).*(getter))();

  typedef BasicPropertyToWidgetDataMapping<
      TAtomic, TWidget, TContainer, WidgetTraits> MappingType;

  MappingType *mapping = new MappingType(w, p, setter, c, traits);

  QtCouplingHelper *h = new QtCouplingHelper(w, mapping);

  // The property should notify the widget of changes
  LatentITKEventNotifier::connect(
        &p, PropertyChangeEvent(),
        h, SLOT(onPropertyModification()));

  // Coupling helper listens to events from widget
  h->connect(w, traits.GetSignal(), SLOT(onUserModification()));
}

template <class TAtomic, class TWidget, class TContainer>
void makeCoupling(TWidget *w, TContainer *c,
                  void (TContainer::*setter)(TAtomic),
                  ConstProperty<TAtomic> & (TContainer::*getter)())
{
  typedef DefaultWidgetTraits<TAtomic, TWidget> WidgetTraits;
  WidgetTraits traits;
  makeCoupling<TAtomic, TWidget, TContainer, WidgetTraits>(
        w,c,setter,getter, traits);
}


template <class TAtomic, class TWidget, class WidgetTraits>
void makeCoupling(
    TWidget *w,
    AbstractEditableNumericValueModel<TAtomic> *model,
    WidgetTraits traits = DefaultWidgetTraits<TAtomic, TWidget>())
{
  typedef NumericModelWidgetDataMapping<
      TAtomic, TWidget *, WidgetTraits> MappingType;
  MappingType *mapping = new MappingType(w, model, traits);
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
  h->connect(w, traits.GetSignal(), SLOT(onUserModification()));
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
void makeArrayCoupling(
    std::vector<TWidget *> wa,
    AbstractEditableNumericValueModel< iris_vector_fixed<TAtomic, VDim> > *model,
    WidgetTraits traits)
{
  typedef std::vector<TWidget *> WidgetArray;
  typedef WidgetArrayTraits<TAtomic, VDim, TWidget, WidgetTraits> ArrayTraits;
  typedef iris_vector_fixed<TAtomic, VDim> ValueType;
  typedef NumericModelWidgetDataMapping<
      ValueType, WidgetArray, ArrayTraits> MappingType;

  // Create the mapping
  MappingType *mapping = new MappingType(wa, model, ArrayTraits(traits));

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
    h->connect(wa[i], traits.GetSignal(), SLOT(onUserModification()));
}

template <class TAtomic, class TWidget, class WidgetTraits>
void makeArrayCoupling(
    TWidget *w1, TWidget *w2, TWidget *w3,
    AbstractEditableNumericValueModel<iris_vector_fixed<TAtomic, 3> > *model,
    WidgetTraits traits)
{
  // Create the array of widgets
  std::vector<TWidget *> wa(3);
  wa[0] = w1; wa[1] = w2; wa[2] = w3;

  // Call the main method
  makeArrayCoupling<3, TAtomic, TWidget, WidgetTraits>(wa, model, traits);
}

template <class TAtomic, class TWidget>
void makeArrayCoupling(
    TWidget *w1, TWidget *w2, TWidget *w3,
    AbstractEditableNumericValueModel<iris_vector_fixed<TAtomic, 3> > *model)
{
  // Create the default traits
  DefaultWidgetTraits<TAtomic, TWidget> traits;

  // Call the main method
  makeArrayCoupling(w1, w2, w3, model, traits);
}


template <class TAtomic, class TWidget, class WidgetTraits>
void makeArrayCoupling(
    TWidget *w1, TWidget *w2,
    AbstractEditableNumericValueModel<iris_vector_fixed<TAtomic, 2> > *model,
    WidgetTraits traits)
{
  // Create the array of widgets
  std::vector<TWidget *> wa(2);
  wa[0] = w1; wa[1] = w2;

  // Call the main method
  makeArrayCoupling<2, TAtomic, TWidget, WidgetTraits>(wa, model, traits);
}


template <class TAtomic, class TWidget>
void makeArrayCoupling(
    TWidget *w1, TWidget *w2,
    AbstractEditableNumericValueModel<iris_vector_fixed<TAtomic, 2> > *model)
{
  // Create the default traits
  DefaultWidgetTraits<TAtomic, TWidget> traits;

  // Call the main method
  makeArrayCoupling(w1, w2, model, traits);
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
  WidgetTraits traits;
  MappingType *mapping = new MappingType(w, model, traits);
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
