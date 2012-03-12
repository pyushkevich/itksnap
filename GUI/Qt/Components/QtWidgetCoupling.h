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
#include <PropertyModel.h>
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
template <class TAtomic, class TDomain, class TWidgetPtr,
          class WidgetValueTraits, class WidgetDomainTraits>
class PropertyModelToWidgetDataMapping : public AbstractWidgetDataMapping
{
public:
  // The model that provides the data
  typedef AbstractPropertyModel<TAtomic, TDomain> ModelType;

  // Constructor
  PropertyModelToWidgetDataMapping(
      TWidgetPtr w, ModelType *model,
      WidgetValueTraits valueTraits, WidgetDomainTraits domainTraits)
    : m_Widget(w), m_Model(model), m_Updating(false),
      m_ValueTraits(valueTraits), m_DomainTraits(domainTraits) {}

  // Populate widget
  void CopyFromTargetToWidget()
  {
    m_Updating = true;

    // Prepopulate the range with current values in case the model does
    // not actually compute ranges
    TDomain domain = m_DomainTraits.GetDomain(m_Widget);
    TAtomic value;

    // Obtain the value from the model
    if(m_Model->GetValueAndDomain(value, &domain))
      {
      m_DomainTraits.SetDomain(m_Widget, domain);
      m_ValueTraits.SetValue(m_Widget, value);
      }
    else
      {
      m_ValueTraits.SetValueToNull(m_Widget);
      }

    m_Updating = false;
  }

  // Get value from widget
  void CopyFromWidgetToTarget()
  {
    if(!m_Updating)
      {
      TAtomic user_value = m_ValueTraits.GetValue(m_Widget);
      TAtomic model_value;

      // Note: if the model reports that the value is invalid, we are not
      // allowing the user to mess with the value. This may have some odd
      // consequences. We need to investigate.
      if(m_Model->GetValueAndDomain(model_value, NULL) &&
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
  WidgetValueTraits m_ValueTraits;
  WidgetDomainTraits m_DomainTraits;
};


/**
  Data mapping between a property and an input widget TWidget.
  The mapping handles the value of the widget only.
  */
template <class TAtomic, class TWidget, class TPropertyContainer,
          class WidgetValueTraits>
class BasicPropertyToWidgetDataMapping : public AbstractWidgetDataMapping
{
public:
  typedef void (TPropertyContainer::*SetterFunction)(TAtomic);

  BasicPropertyToWidgetDataMapping(
      TWidget *w, ConstProperty<TAtomic> &p,
      SetterFunction setter, TPropertyContainer *c,
      WidgetValueTraits traits)
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
  WidgetValueTraits m_Traits;
};


/**
  Base class for widget value traits
  */
template <class TAtomic, class TWidgetPtr>
class WidgetValueTraitsBase
{
public:
  // Get the Qt signal that the widget fires when its value has changed
  virtual const char *GetSignal() { return NULL; }

  // Get the value from the widget
  virtual TAtomic GetValue(TWidgetPtr) = 0;

  // Set the value of the widget
  virtual void SetValue(TWidgetPtr, const TAtomic &) = 0;

  // The default behavior for setting the widget to null is to do nothing.
  // This should be overridden by child traits classes
  virtual void SetValueToNull(TWidgetPtr) {}
};

/**
  Base class for widget domain traits
  */
template <class TDomain, class TWidgetPtr>
class WidgetDomainTraitsBase
{
public:
  virtual void SetDomain(TWidgetPtr w, const TDomain &domain) = 0;
  virtual TDomain GetDomain(TWidgetPtr w) = 0;
};


/**
  Empty template for default traits. Specialize for different Qt widgets
  */
template <class TAtomic, class TWidget>
class DefaultWidgetValueTraits
{
};

template <class TDomain, class TWidget>
class DefaultWidgetDomainTraits
{
};

template <class TWidget>
class DefaultWidgetDomainTraits<TrivialDomain, TWidget>
    : public WidgetDomainTraitsBase<TrivialDomain, TWidget *>
{
public:
  // With a trivial domain, do nothing!
  virtual void SetDomain(TWidget *w, const TrivialDomain &) {}

  virtual TrivialDomain GetDomain(TWidget *w)
  { return TrivialDomain(); }
};


/**
  Default traits for the Qt Spin Box, coupled with a numeric value range
  */
template <class TAtomic>
class DefaultWidgetValueTraits<TAtomic, QSpinBox>
    : public WidgetValueTraitsBase<TAtomic, QSpinBox *>
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


  void SetValueToNull(QSpinBox *w)
  {
    w->setValue(w->minimum());
    w->setSpecialValueText(" ");
  }

};

template <class TAtomic>
class DefaultWidgetDomainTraits<NumericValueRange<TAtomic>, QSpinBox>
    : public WidgetDomainTraitsBase<NumericValueRange<TAtomic>, QSpinBox *>
{
public:

  void SetDomain(QSpinBox *w, const NumericValueRange<TAtomic> &range)
  {
    w->setMinimum(range.Minimum);
    w->setMaximum(range.Maximum);
    w->setSingleStep(range.StepSize);
  }

  NumericValueRange<TAtomic> GetDomain(QSpinBox *w)
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
class DefaultWidgetValueTraits<TAtomic, QDoubleSpinBox>
    : public WidgetValueTraitsBase<TAtomic, QDoubleSpinBox *>
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


  void SetValueToNull(QDoubleSpinBox *w)
  {
    w->setValue(w->minimum());
    w->setSpecialValueText(" ");
  }
};

template <class TAtomic>
class DefaultWidgetDomainTraits<NumericValueRange<TAtomic>, QDoubleSpinBox>
    : public WidgetDomainTraitsBase<NumericValueRange<TAtomic>, QDoubleSpinBox *>
{
public:
  void SetDomain(QDoubleSpinBox *w, const NumericValueRange<TAtomic> &range)
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

  NumericValueRange<TAtomic> GetDomain(QDoubleSpinBox *w)
  {
    return NumericValueRange<TAtomic>(
          static_cast<TAtomic>(w->minimum()),
          static_cast<TAtomic>(w->maximum()),
          static_cast<TAtomic>(w->singleStep()));
  }
};


template <class TAtomic>
class DefaultWidgetValueTraits<TAtomic, QSlider>
    : public WidgetValueTraitsBase<TAtomic, QSlider *>
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
class DefaultWidgetValueTraits<TAtomic, QLineEdit>
    : public WidgetValueTraitsBase<TAtomic, QLineEdit *>
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
    : public DefaultWidgetValueTraits<TAtomic, QLineEdit>
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
struct DefaultWidgetValueTraits<TAtomic, QCheckBox>
    : public WidgetValueTraitsBase<TAtomic, QCheckBox *>
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
/*
template <class TAtomic>
struct DefaultWidgetValueTraits<TAtomic, QRadioButton>
    : public WidgetValueTraitsBase<TAtomic, QRadioButton *>

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
*/

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
  This class allows widget traits to be extended to an array of widgets. It
  uses a child traits object to map between an iris_vector_fixed and an array
  of widgets.
  */
template <class TAtomic, unsigned int VDim, class TWidget, class ChildTraits>
class WidgetArrayValueTraits :
    public WidgetValueTraitsBase< iris_vector_fixed<TAtomic, VDim>,
                                  std::vector<TWidget *> >
{
public:
  typedef iris_vector_fixed<TAtomic, VDim> ValueType;
  typedef std::vector<TWidget *> WidgetArrayType;

  /**
    Constructor, takes the "child" traits object, i.e., the traits for the
    individual widgets in the array
    */
  WidgetArrayValueTraits(ChildTraits childTraits)
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
  ChildTraits m_ChildTraits;
};

/**
  Before defining a vectorized domain traits object, we need to define some
  traits that describe how different kinds of domains map between atomic
  and vectorized versions.
  */
template <class TDomain, unsigned int VDim>
class DomainVectorExpansion
{

};

template <class TAtomic, unsigned int VDim>
class DomainVectorExpansion<NumericValueRange<TAtomic>, VDim>
{
public:
  typedef iris_vector_fixed<TAtomic, VDim> VectorType;
  typedef NumericValueRange<VectorType> VectorDomainType;
  typedef NumericValueRange<TAtomic> AtomicDomainType;

  static AtomicDomainType GetNthElement(
      const VectorDomainType &dvec, unsigned int n)
  {
    return AtomicDomainType(
          dvec.Minimum(n), dvec.Maximum(n), dvec.StepSize(n));
  }

  static void UpdateNthElement(
      VectorDomainType &dvec, unsigned int n, const AtomicDomainType &dat)
  {
    dvec.Minimum(n) = dat.Minimum;
    dvec.Maximum(n) = dat.Maximum;
    dvec.StepSize(n) = dat.StepSize;
  }
};

template <unsigned int VDim>
class DomainVectorExpansion<TrivialDomain, VDim>
{
public:
  typedef TrivialDomain VectorDomainType;
  typedef TrivialDomain AtomicDomainType;

  static AtomicDomainType GetNthElement(
      const VectorDomainType &dvec, unsigned int n)
  {
    return AtomicDomainType();
  }

  static void UpdateNthElement(
      VectorDomainType &dvec, unsigned int n, const AtomicDomainType &dat) {}
};


template <class TVectorDomain, int VDim>
class ComponentDomainTraits
{
};

template <class TAtomic, int VDim>
class ComponentDomainTraits<
    NumericValueRange<iris_vector_fixed<TAtomic, VDim> >, VDim>
{
public:
  typedef iris_vector_fixed<TAtomic, VDim> VectorType;
  typedef NumericValueRange<TAtomic> AtomicDomainType;
  typedef NumericValueRange<VectorType> VectorDomainType;

  static AtomicDomainType GetNthElement(
      const VectorDomainType &dvec, unsigned int n)
  {
    return AtomicDomainType(
          dvec.Minimum(n), dvec.Maximum(n), dvec.StepSize(n));
  }

  static void UpdateNthElement(
      VectorDomainType &dvec, unsigned int n, const AtomicDomainType &dat)
  {
    dvec.Minimum(n) = dat.Minimum;
    dvec.Maximum(n) = dat.Maximum;
    dvec.StepSize(n) = dat.StepSize;
  }
};

template <int VDim>
class ComponentDomainTraits<TrivialDomain, VDim>
{
public:
  typedef TrivialDomain AtomicDomainType;
  typedef TrivialDomain VectorDomainType;

  static AtomicDomainType GetNthElement(
      const VectorDomainType &dvec, unsigned int n)
  {
    return AtomicDomainType();
  }

  static void UpdateNthElement(
      VectorDomainType &dvec, unsigned int n, const AtomicDomainType &dat) {}
};


template <class TVectorDomain, unsigned int VDim,
          class TWidget, class ChildTraits>
class WidgetArrayDomainTraits :
    public WidgetDomainTraitsBase<TVectorDomain, std::vector<TWidget *> >
{
public:
  typedef ComponentDomainTraits<TVectorDomain, VDim> ComponentTraitsType;
  typedef TVectorDomain VectorDomainType;
  typedef typename ComponentTraitsType::AtomicDomainType AtomicDomainType;
  typedef std::vector<TWidget *> WidgetArrayType;

  /**
    Constructor, takes the "child" traits object, i.e., the traits for the
    individual widgets in the array
    */
  WidgetArrayDomainTraits(ChildTraits childTraits)
    : m_ChildTraits(childTraits) {}

  void SetDomain(WidgetArrayType wa, const VectorDomainType &range)
  {
    for(unsigned int i = 0; i < VDim; i++)
      {
      AtomicDomainType di = ComponentTraitsType::GetNthElement(range, i);
      m_ChildTraits.SetDomain(wa[i], di);
      }
  }

  VectorDomainType GetDomain(WidgetArrayType wa)
  {
    VectorDomainType range;
    for(unsigned int i = 0; i < VDim; i++)
      {
      AtomicDomainType ri = m_ChildTraits.GetDomain(wa[i]);
      ComponentTraitsType::UpdateNthElement(range, i, ri);
      }
    return range;
  }

protected:
  ChildTraits m_ChildTraits;
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

template <class TAtomic, class TWidget, class TContainer,
          class WidgetValueTraits>
void makeCoupling(TWidget *w, TContainer *c,
                  void (TContainer::*setter)(TAtomic),
                  ConstProperty<TAtomic> & (TContainer::*getter)(),
                  WidgetValueTraits valueTraits)
{
  // Retrieve the property
  ConstProperty<TAtomic> &p = ((*c).*(getter))();

  typedef BasicPropertyToWidgetDataMapping<
      TAtomic, TWidget, TContainer, WidgetValueTraits> MappingType;

  MappingType *mapping = new MappingType(w, p, setter, c, valueTraits);

  QtCouplingHelper *h = new QtCouplingHelper(w, mapping);

  // The property should notify the widget of changes
  LatentITKEventNotifier::connect(
        &p, PropertyChangeEvent(),
        h, SLOT(onPropertyModification()));

  // Coupling helper listens to events from widget
  h->connect(w, valueTraits.GetSignal(), SLOT(onUserModification()));
}

template <class TAtomic, class TWidget, class TContainer>
void makeCoupling(TWidget *w, TContainer *c,
                  void (TContainer::*setter)(TAtomic),
                  ConstProperty<TAtomic> & (TContainer::*getter)())
{
  typedef DefaultWidgetValueTraits<TAtomic, TWidget> WidgetValueTraits;
  WidgetValueTraits valueTraits;
  makeCoupling<TAtomic, TWidget, TContainer, WidgetValueTraits>(
        w,c,setter,getter, valueTraits);
}


template <class TModel, class TWidget,
          class WidgetValueTraits, class WidgetDomainTraits>
void makeCoupling(
    TWidget *w,
    TModel *model,
    WidgetValueTraits valueTraits,
    WidgetDomainTraits domainTraits)
{
  typedef typename TModel::ValueType ValueType;
  typedef typename TModel::DomainType DomainType;

  typedef PropertyModelToWidgetDataMapping<
      ValueType, DomainType, TWidget *,
      WidgetValueTraits, WidgetDomainTraits> MappingType;

  MappingType *mapping = new MappingType(w, model, valueTraits, domainTraits);

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
  h->connect(w, valueTraits.GetSignal(), SLOT(onUserModification()));
}

template <class TModel, class TWidget, class WidgetValueTraits>
void makeCoupling(TWidget *w,
                  TModel *model,
                  WidgetValueTraits trValue)
{
  typedef typename TModel::DomainType DomainType;
  typedef DefaultWidgetDomainTraits<DomainType, TWidget> WidgetDomainTraits;
  makeCoupling<TModel, TWidget,
      WidgetValueTraits, WidgetDomainTraits>(
        w, model, trValue, WidgetDomainTraits());
}


/**
  Create a coupling between a numeric model and a Qt widget. The widget
  will listen to the events from the model and update its value and range
  accordingly. When the user interacts with the widget, the model will be
  updated. The coupling fully automates mapping of data between Qt input
  widgets and SNAP numeric models.

  This version of the method uses default traits. There is also a version
  that allows you to provide your own traits.
*/
template <class TModel, class TWidget>
void makeCoupling(TWidget *w,
                  TModel *model)
{
  typedef typename TModel::ValueType ValueType;
  typedef DefaultWidgetValueTraits<ValueType, TWidget> WidgetValueTraits;
  makeCoupling<TModel, TWidget,WidgetValueTraits>(
        w, model, WidgetValueTraits());
}



/**
  Create a coupling between an model whose value is of a vector type and
  an array of widgets of the same type. For example, this function allows
  you to hook up a model wrapped around a Vector3d to a triple of spin boxes.
  This is very convenient for dealing with input and output of vector data.
  */

template <class TModel, class TWidget>
class DefaultComponentValueTraits : public DefaultWidgetValueTraits<
  typename TModel::ValueType::element_type, TWidget>
{
};

template <class TModel, class TWidget>
class DefaultComponentDomainTraits : public DefaultWidgetDomainTraits<
    typename ComponentDomainTraits<typename TModel::DomainType,
                          TModel::ValueType::SIZE>::AtomicDomainType,
    TWidget>
{
};


/**
  Create a coupling between a model and an array of widgets. See the more
  convenient versions of this method below
  */
template <class TWidget,
          class TModel,
          class WidgetValueTraits,
          class WidgetDomainTraits>
void makeWidgetArrayCoupling(
    std::vector<TWidget *> wa,
    TModel *model,
    WidgetValueTraits trValue,
    WidgetDomainTraits trDomain)
{
  typedef std::vector<TWidget *> WidgetArray;
  typedef typename TModel::ValueType VectorType;
  typedef typename TModel::DomainType VectorDomainType;
  typedef typename VectorType::element_type ElementType;
  const int VDim = VectorType::SIZE;

  // Define the array traits
  typedef WidgetArrayValueTraits<
      ElementType, VDim, TWidget, WidgetValueTraits> ArrayValueTraits;

  typedef WidgetArrayDomainTraits<
      VectorDomainType, VDim, TWidget, WidgetDomainTraits> ArrayDomainTraits;

  // The class of the mapping
  typedef PropertyModelToWidgetDataMapping<
      VectorType, VectorDomainType, WidgetArray,
      ArrayValueTraits, ArrayDomainTraits> MappingType;

  // Create the mapping
  MappingType *mapping = new MappingType(
        wa, model,
        ArrayValueTraits(trValue),
        ArrayDomainTraits(trDomain));

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
  for(int i = 0; i < VDim; i++)
    h->connect(wa[i], trValue.GetSignal(), SLOT(onUserModification()));
}

/**
  Create a coupling between a model and a triplet of widgets. The model must
  be an AbstractPropertyModel templated over iris_vector_fixed<T,3> and some
  compatible Domain object, i.e., NumericValueRange or TrivialDomain. The
  caller can optionally pass traits objects for overriding the default behavior
  of model-to-widget copying of values and domain information.
  */
template <class TModel, class TWidget,
          class WidgetValueTraits, class WidgetDomainTraits>
void makeArrayCoupling(
    TWidget *w1, TWidget *w2, TWidget *w3,
    TModel *model,
    WidgetValueTraits trValue,
    WidgetDomainTraits trDomain)
{
  // Create the array of widgets
  typedef std::vector<TWidget *> WidgetArray;
  WidgetArray wa(3);
  wa[0] = w1; wa[1] = w2; wa[2] = w3;

  // Call the parent method
  makeWidgetArrayCoupling<
      TWidget,TModel,WidgetValueTraits,WidgetDomainTraits>(
        wa, model, trValue, trDomain);
}

template <class TModel, class TWidget,
          class WidgetValueTraits>
void makeArrayCoupling(
    TWidget *w1, TWidget *w2, TWidget *w3,
    TModel *model,
    WidgetValueTraits trValue)
{
  typedef DefaultComponentDomainTraits<TModel,TWidget> WidgetDomainTraits;
  makeArrayCoupling<TModel, TWidget, WidgetValueTraits, WidgetDomainTraits>
      (w1,w2,w3,model,trValue,WidgetDomainTraits());
}

template <class TModel, class TWidget>
void makeArrayCoupling(
    TWidget *w1, TWidget *w2, TWidget *w3,
    TModel *model)
{
  typedef DefaultComponentValueTraits<TModel,TWidget> WidgetValueTraits;
  makeArrayCoupling<TModel, TWidget, WidgetValueTraits>
      (w1,w2,w3,model,WidgetValueTraits());
}


/**
  Create a coupling between a model and a pair of widgets. The model must
  be an AbstractPropertyModel templated over iris_vector_fixed<T,2> and some
  compatible Domain object, i.e., NumericValueRange or TrivialDomain. The
  caller can optionally pass traits objects for overriding the default behavior
  of model-to-widget copying of values and domain information.
  */
template <class TModel, class TWidget,
          class WidgetValueTraits, class WidgetDomainTraits>
void makeArrayCoupling(
    TWidget *w1, TWidget *w2,
    TModel *model,
    WidgetValueTraits trValue,
    WidgetDomainTraits trDomain)
{
  // Create the array of widgets
  typedef std::vector<TWidget *> WidgetArray;
  WidgetArray wa(2);
  wa[0] = w1; wa[1] = w2;

  // Call the parent method
  makeWidgetArrayCoupling<
      TWidget,TModel,WidgetValueTraits,WidgetDomainTraits>(
        wa, model, trValue, trDomain);
}

template <class TModel, class TWidget,
          class WidgetValueTraits>
void makeArrayCoupling(
    TWidget *w1, TWidget *w2,
    TModel *model,
    WidgetValueTraits trValue)
{
  typedef DefaultComponentDomainTraits<TModel,TWidget> WidgetDomainTraits;
  makeArrayCoupling<TModel, TWidget, WidgetValueTraits, WidgetDomainTraits>
      (w1,w2,model,trValue,WidgetDomainTraits());
}

template <class TModel, class TWidget>
void makeArrayCoupling(
    TWidget *w1, TWidget *w2,
    TModel *model)
{
  typedef DefaultComponentValueTraits<TModel,TWidget> WidgetValueTraits;
  makeArrayCoupling<TModel, TWidget, WidgetValueTraits>
      (w1,w2,model,WidgetValueTraits());
}



/**
  Create a coupling between a widget containing a set of radio buttons
  and an enum. The values of the enum must be 0,1,2,... The buttons must
  be in the same order as the enum entries.
  */
template <class TAtomic, class TWidget>
void makeRadioGroupCoupling(
    TWidget *w, AbstractPropertyModel<TAtomic> *model)
{
  typedef RadioButtonGroupTraits<TAtomic> WidgetValueTraits;
  typedef DefaultWidgetDomainTraits<TrivialDomain, TWidget> WidgetDomainTraits;

  typedef PropertyModelToWidgetDataMapping<
      TAtomic, TrivialDomain, TWidget *,
      WidgetValueTraits, WidgetDomainTraits> MappingType;

  WidgetValueTraits valueTraits;
  WidgetDomainTraits domainTraits;
  MappingType *mapping = new MappingType(w, model, valueTraits, domainTraits);
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
