#include "IRISObserverPattern.h"
#include "itkObject.h"

IRISObservable::IRISObservable()
{
  m_Implementation = itk::Object::New();
}

unsigned long IRISObservable
::AddObserver(const itk::EventObject &ev, itk::Command *command)
{
  return m_Implementation->AddObserver(ev, command);
}

void
IRISObservable::RemoveOvserver(unsigned long tag)
{
  m_Implementation->RemoveObserver(tag);
}

void
IRISObservable::InvokeEvent(const itk::EventObject &ev)
{
  m_Implementation->InvokeEvent(ev);
}
