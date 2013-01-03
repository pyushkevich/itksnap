#include "StateManagement.h"
#include "SNAPEvents.h"
#include "SNAPEventListenerCallbacks.h"

void BooleanCondition
::OnStateChange()
{
  InvokeEvent(StateMachineChangeEvent());
}

BinaryBooleanCondition
::BinaryBooleanCondition(BooleanCondition *a, BooleanCondition *b)
{
  m_A = a;
  m_B = b;

  // Make sure events propagate up
  AddListener<BooleanCondition>(m_A, StateMachineChangeEvent(),
                                this, &BooleanCondition::OnStateChange);

  AddListener<BooleanCondition>(m_B, StateMachineChangeEvent(),
                                this, &BinaryBooleanCondition::OnStateChange);
}

AndCondition::AndCondition(BooleanCondition *a, BooleanCondition *b)
  : BinaryBooleanCondition(a,b)
{
}

bool AndCondition::operator ()() const
{
  return (*m_A)() && (*m_B)();
}

SmartPtr<AndCondition>
AndCondition::New(BooleanCondition *a, BooleanCondition *b)
{
  SmartPtr<AndCondition> p = new AndCondition(a, b);
  p->UnRegister();
  return p;
}

OrCondition::OrCondition(BooleanCondition *a, BooleanCondition *b)
  : BinaryBooleanCondition(a,b)
{
}

bool OrCondition::operator ()() const
{
  return (*m_A)() || (*m_B)();
}

SmartPtr<OrCondition>
OrCondition::New(BooleanCondition *a, BooleanCondition *b)
{
  SmartPtr<OrCondition> p = new OrCondition(a, b);
  p->UnRegister();
  return p;
}

NotCondition::NotCondition(BooleanCondition *a)
{
  m_A = a;
  AddListener<BooleanCondition>(m_A, StateMachineChangeEvent(),
                                this, &BooleanCondition::OnStateChange);
}

bool NotCondition::operator ()() const
{
  return ! (*m_A)();
}

SmartPtr<NotCondition>
NotCondition::New(BooleanCondition *a)
{
  SmartPtr<NotCondition> p = new NotCondition(a);
  p->UnRegister();
  return p;
}
