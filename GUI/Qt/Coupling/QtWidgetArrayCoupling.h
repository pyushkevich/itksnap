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

#ifndef QTWIDGETARRAYCOUPLING_H
#define QTWIDGETARRAYCOUPLING_H

#include <QtWidgetCoupling.h>

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
    : m_ChildTraits(childTraits) { m_CacheValid.fill(false); }

  ValueType GetValue(WidgetArrayType wa)
  {
    ValueType value = m_CachedModelValue;
    for(unsigned int i = 0; i < VDim; i++)
      {
      TAtomic valWidget = m_ChildTraits.GetValue(wa[i]);
      if(!m_CacheValid[i] || valWidget != m_CachedWidgetValue[i])
        {
        value(i) = valWidget;
        m_CacheValid[i] = false;
        }
      }
    return value;
  }

  void SetValue(WidgetArrayType wa, const ValueType &value)
  {
    for(unsigned int i = 0; i < VDim; i++)
      {
      m_ChildTraits.SetValue(wa[i], value(i));
      m_CachedModelValue[i] = value(i);
      m_CachedWidgetValue[i] = m_ChildTraits.GetValue(wa[i]);
      m_CacheValid[i] = true;
      }
  }

  void SetValueToNull(WidgetArrayType wa)
  {
    for(unsigned int i = 0; i < VDim; i++)
      m_ChildTraits.SetValueToNull(wa[i]);
    m_CacheValid.fill(false);
  }

  const char *GetSignal()
  {
    return m_ChildTraits.GetSignal();
  }

protected:
  ChildTraits m_ChildTraits;

  // We must cache the values sent to each widget and the corresponding states
  // of each widget. This is because some widgets change the value passed to
  // them. If one of the widgets in the array is edited by the user, we only
  // want that widget's value to be sent to the model, while using the cached
  // values for the other widgets.
  ValueType m_CachedModelValue, m_CachedWidgetValue;
  iris_vector_fixed<bool, VDim> m_CacheValid;
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
      TModel, WidgetArray,
      ArrayValueTraits, ArrayDomainTraits> MappingType;

  // Create the mapping
  MappingType *mapping = new MappingType(
        wa, model,
        ArrayValueTraits(trValue),
        ArrayDomainTraits(trDomain));

  // Create the coupling helper (event handler). It's attached to the first
  // widget, just for the purpose of this object being deleted later.
  QtCouplingHelper *h = new QtCouplingHelper(wa.front(), mapping);

  // Populate the widget (force the domain and value to be copied)
  mapping->InitializeWidgetFromModel();

  // Listen to value change events from the model
  LatentITKEventNotifier::connect(
        model, ValueChangedEvent(),
        h, SLOT(onPropertyModification(const EventBucket &)));

  LatentITKEventNotifier::connect(
        model, DomainChangedEvent(),
        h, SLOT(onPropertyModification(const EventBucket &)));

  LatentITKEventNotifier::connect(
        model, DomainDescriptionChangedEvent(),
        h, SLOT(onPropertyModification(const EventBucket &)));

  // Listen to value change events for this widget
  for(int i = 0; i < VDim; i++)
    {
    h->connect(trValue.GetSignalEmitter(wa[i]),
               trValue.GetSignal(), SLOT(onUserModification()));
    }
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



#endif // QTWIDGETARRAYCOUPLING_H
