#ifndef IRISOBSERVERPATTERN_H
#define IRISOBSERVERPATTERN_H

#include <SNAPCommon.h>
#include <itkCommand.h>

namespace itk
{
class Object;
class EventObject;
}

class IRISObservable
{
public:
  IRISObservable();
  unsigned long AddObserver(const itk::EventObject &ev, itk::Command *command);
  void RemoveOvserver(unsigned long tag);

protected:
  void InvokeEvent(const itk::EventObject &ev);

  // For the time being, implementation is through itk::Object. This could be
  // made more light weight by having our own implementation
  SmartPtr<itk::Object> m_Implementation;
};


template <class TObserver>
unsigned long AddListener(IRISObservable *sender,
                 const itk::EventObject &event,
                 TObserver *observer,
                 void (TObserver::*memberFunction)())
{
  typedef itk::SimpleMemberCommand<TObserver> Cmd;
  typename Cmd::Pointer cmd = Cmd::New();
  cmd->SetCallbackFunction(observer, memberFunction);
  return sender->AddObserver(event, cmd);
}


template <class TObserver>
unsigned long AddListener(IRISObservable *sender,
                 const itk::EventObject &event,
                 TObserver *observer,
                 void (TObserver::*memberFunction)(itk::Object*, const itk::EventObject &))
{
  typedef itk::MemberCommand<TObserver> Cmd;
  typename Cmd::Pointer cmd = Cmd::New();
  cmd->SetCallbackFunction(observer, memberFunction);
  return sender->AddObserver(event, cmd);
}


#endif // IRISOBSERVERPATTERN_H
