#ifndef SNAPUIFLAG_H
#define SNAPUIFLAG_H

#include "StateManagement.h"
#include "UIState.h"
#include "SNAPEventListenerCallbacks.h"

class GlobalUIModel;

/**
  A BooleanCondition implementation that queries a TModel about different
  UI states of type TStateEnum. The Model must implement a function
  bool CheckState(TStateEnum state), which is called to check the state.
  The model must also fire an event StateMachineChangeEvent() in order
  for the activation machinery to work.

  The implementation of this class is in a .txx file to allow use with
  not yet known Models and Enums.
  */
template<class TModel, class TStateEnum>
class SNAPUIFlag : public BooleanCondition
{
public:
  typedef SNAPUIFlag<TModel, TStateEnum> Self;
  typedef BooleanCondition Superclass;
  itkTypeMacro(Self, Superclass)

  static SmartPtr<Self> New(TModel *model, TStateEnum state)
  {
    SmartPtr<Self> p = new Self(model, state);
    p->UnRegister();
    return p;
  }

  bool operator() () const
  {
    if(m_Model)
      return m_Model->CheckState(m_State);
    else return false;
  }

protected:

  SNAPUIFlag(TModel *model, TStateEnum state)
  {
    m_Model = model;
    m_State = state;

    m_Tag = AddListener<Self>(
          m_Model, StateMachineChangeEvent(),
          this, &Self::OnStateChange);

    m_DeleteTag = AddListener<Self>(
          m_Model, itk::DeleteEvent(),
          this, &Self::OnModelDeletion);
  }

  virtual ~SNAPUIFlag()
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
  TStateEnum m_State;
  unsigned long m_Tag, m_DeleteTag;
};

#endif // SNAPUIFLAG_H
