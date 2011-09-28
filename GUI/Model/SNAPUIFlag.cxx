#include "SNAPUIFlag.h"
#include "GlobalUIModel.h"

SNAPUIFlag::SNAPUIFlag(GlobalUIModel *model, UIState state)
{
  m_Model = model;
  m_State = state;
  AddListener<SNAPUIFlag>(
        m_Model, StateMachineChangeEvent(),
        this, &SNAPUIFlag::OnStateChange);
}

bool SNAPUIFlag::operator() () const
{
  return m_Model->checkState(m_State);
}

SmartPtr<SNAPUIFlag>
SNAPUIFlag::New(GlobalUIModel *model, UIState state)
{
  SmartPtr<SNAPUIFlag> p = new SNAPUIFlag(model, state);
  p->UnRegister();
  return p;
}
