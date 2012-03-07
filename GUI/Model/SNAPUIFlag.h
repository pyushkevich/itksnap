#ifndef SNAPUIFLAG_H
#define SNAPUIFLAG_H

#include "StateManagement.h"
#include "UIState.h"

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
    return m_Model->CheckState(m_State);
  }

protected:

  SNAPUIFlag(TModel *model, TStateEnum state)
  {
    m_Model = model;
    m_State = state;
    AddListener<Self>(
          m_Model, StateMachineChangeEvent(),
          this, &Self::OnStateChange);
  }

  virtual ~SNAPUIFlag() {}

private:
  TModel *m_Model;
  TStateEnum m_State;
};

#endif // SNAPUIFLAG_H
