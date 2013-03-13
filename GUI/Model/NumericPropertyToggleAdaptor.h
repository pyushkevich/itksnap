#ifndef NUMERICPROPERTYTOGGLEADAPTOR_H
#define NUMERICPROPERTYTOGGLEADAPTOR_H

#include "PropertyModel.h"

/**
 * @brief The NumericPropertyToggleAdaptor class
 *
 * This model is a wrapper around a numerical property that allows the user
 * to toggle the property on and off. When the property is toggled off, it is
 * set to the DefaultOffValue, when it is toggled on, it is set to the last
 * value when it was toggled off.
 *
 * The main intended use of this adapter is for opacity sliders
 *
 * This model necessarily has an ambiguity when the user moves the slider to
 * the off value, and then toggles the switch to the on position. The model
 * sets the slider to the DefaultOnValue when that happens.
 *
 * The model is templated over the model representing the numerical value.
 * Typically, this would be ConcreteRangedIntProperty or a similar class.
 */

template <class TNumericValueModel>
class NumericPropertyToggleAdaptor : public AbstractSimpleBooleanProperty
{
public:
  typedef NumericPropertyToggleAdaptor<TNumericValueModel> Self;
  typedef AbstractSimpleBooleanProperty Superclass;
  typedef SmartPtr<Self> Pointer;
  typedef SmartPtr<const Self> ConstPointer;
  itkTypeMacro(NumericPropertyToggleAdaptor, AbstractSimpleBooleanProperty)
  itkNewMacro(Self)

  typedef TNumericValueModel ValueModelType;
  typedef typename ValueModelType::ValueType NumericValueType;
  typedef typename Superclass::ValueType ValueType;
  typedef typename Superclass::DomainType DomainType;

  /** Set the default value when going from off state to on state */
  void SetDefaultOnValue(NumericValueType value)
    { m_DefaultOnValue = value; }

  /** The default off value should be zero, but can be changed */
  void SetDefaultOffValue(NumericValueType value)
    { m_DefaultOffValue = value; }

  /** Set the wrapped value property model */
  void SetWrappedValueModel(ValueModelType *model)
  {
    m_ValueModel = model;
    Rebroadcast(model, ValueChangedEvent(), ValueChangedEvent());
  }

  virtual bool GetValueAndDomain(bool &value, TrivialDomain *)
  {
    if(!m_ValueModel) return false;

    NumericValueType numValue;
    if(!m_ValueModel->GetValueAndDomain(numValue, NULL))
      return false;

    value = (numValue != m_DefaultOffValue);
    return true;
  }

  virtual void SetValue(bool value)
  {
    if(!m_ValueModel) return;

    NumericValueType numValue;
    if(!m_ValueModel->GetValueAndDomain(numValue, NULL))
      return;

    if(value && numValue == m_DefaultOffValue)
      {
      // Restore the value to what it was before we clicked the checkbox off
      m_ValueModel->SetValue(m_LastOnValue);
      m_LastOnValue = m_DefaultOnValue;
      }

    if(!value && numValue != m_DefaultOffValue)
      {
      // Save the current value before setting to default
      m_LastOnValue = numValue;
      m_ValueModel->SetValue(m_DefaultOffValue);
      }
  }

protected:
  NumericPropertyToggleAdaptor()
  {
    m_DefaultOffValue = 0;
    m_DefaultOnValue = 0;
    m_LastOnValue = 0;
  }

  NumericValueType m_DefaultOnValue, m_DefaultOffValue, m_LastOnValue;
  SmartPtr<ValueModelType> m_ValueModel;
};


/**
 * A factory method to create these models
 */
template <class TNumericValueModel>
SmartPtr<AbstractSimpleBooleanProperty>
NewNumericPropertyToggleAdaptor(
    TNumericValueModel *model,
    typename TNumericValueModel::ValueType defaultOffValue,
    typename TNumericValueModel::ValueType defaultOnValue)
{
  typedef NumericPropertyToggleAdaptor<TNumericValueModel> Prop;
  SmartPtr<Prop> p = Prop::New();
  p->SetWrappedValueModel(model);
  p->SetDefaultOffValue(defaultOffValue);
  p->SetDefaultOnValue(defaultOnValue);
  SmartPtr<AbstractSimpleBooleanProperty> pp = p.GetPointer();
  return pp;
}

#endif // NUMERICPROPERTYTOGGLEADAPTOR_H
