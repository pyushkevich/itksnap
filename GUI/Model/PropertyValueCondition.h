#ifndef PROPERTYVALUECONDITION_H
#define PROPERTYVALUECONDITION_H

#include "StateManagement.h"
#include "SNAPEventListenerCallbacks.h"
#include <functional>

/**
  A BooleanCondition that applies a predicate to the value of a property
  model. It is re-evaluated whenever the value changes, so it can drive a
  QtWidgetActivator. For example, to disable a button while a text field
  coupled to a string property holds a URL:

    new QtWidgetActivator(btn, {btn},
      PropertyValueCondition<StringModel>::New(model,
        [](const std::string &s) { return !IsRemoteImageURL(s); }));

  The condition is false while the property is invalid.
  */
template <class TModel>
class PropertyValueCondition : public BooleanCondition
{
public:
  typedef PropertyValueCondition<TModel> Self;
  typedef BooleanCondition Superclass;
  itkTypeMacro(Self, Superclass)

  typedef typename TModel::ValueType ValueType;
  typedef std::function<bool(const ValueType &)> Predicate;

  static SmartPtr<Self> New(TModel *model, Predicate predicate)
  {
    SmartPtr<Self> p = new Self(model, predicate);
    p->UnRegister();
    return p;
  }

  bool operator() () const override
  {
    ValueType value;
    return m_Model && m_Model->GetValueAndDomain(value, NULL) && m_Predicate(value);
  }

protected:

  PropertyValueCondition(TModel *model, Predicate predicate)
    : m_Model(model), m_Predicate(predicate)
  {
    m_Tag = AddListener<Self>(
          m_Model, ValueChangedEvent(), this, &Self::OnStateChange);
    m_DeleteTag = AddListener<Self>(
          m_Model, itk::DeleteEvent(), this, &Self::OnModelDeletion);
  }

  virtual ~PropertyValueCondition()
  {
    if(m_Model)
      {
      m_Model->RemoveObserver(m_Tag);
      m_Model->RemoveObserver(m_DeleteTag);
      }
  }

  virtual void OnModelDeletion()
  {
    m_Model = NULL;
  }

private:
  TModel *m_Model;
  Predicate m_Predicate;
  unsigned long m_Tag, m_DeleteTag;
};

#endif // PROPERTYVALUECONDITION_H
