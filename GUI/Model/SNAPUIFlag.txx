#include "SNAPUIFlag.h"

template<class TModel, class TStateEnum>
SNAPUIFlag<TModel, TStateEnum>
::SNAPUIFlag(TModel *model, TStateEnum state)
{
  m_Model = model;
  m_State = state;
  AddListener<Self>(
        m_Model, StateMachineChangeEvent(),
        this, &Self::OnStateChange);
}

template<class TModel, class TStateEnum>
bool SNAPUIFlag<TModel, TStateEnum>
::operator() () const
{
  return m_Model->CheckState(m_State);
}

template<class TModel, class TStateEnum>
SmartPtr<SNAPUIFlag<TModel, TStateEnum> >
SNAPUIFlag<TModel, TStateEnum>
::New(TModel *model, TStateEnum state)
{
  SmartPtr<Self> p = new Self(model, state);
  p->UnRegister();
  return p;
}
