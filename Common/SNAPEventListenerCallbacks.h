#ifndef SNAPEVENTLISTENERCALLBACKS_H
#define SNAPEVENTLISTENERCALLBACKS_H

#include "itkObject.h"
#include "itkCommand.h"

#include "vtkObject.h"
#include "vtkCallbackCommand.h"
#include "vtkSmartPointer.h"


template <class TObserver>
unsigned long AddListener(itk::Object *sender,
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
unsigned long
AddListener(itk::Object *sender,
                 const itk::EventObject &event,
                 TObserver *observer,
                 void (TObserver::*memberFunction)(itk::Object*, const itk::EventObject &))
{
  typedef itk::MemberCommand<TObserver> Cmd;
  typename Cmd::Pointer cmd = Cmd::New();
  cmd->SetCallbackFunction(observer, memberFunction);

  return sender->AddObserver(event, cmd);
}

template <class TObserver>
unsigned long AddListenerConst(itk::Object *sender,
                 const itk::EventObject &event,
                 TObserver *observer,
                 void (TObserver::*memberFunction)(const itk::Object*, const itk::EventObject &))
{
  typedef itk::MemberCommand<TObserver> Cmd;
  typename Cmd::Pointer cmd = Cmd::New();
  cmd->SetCallbackFunction(observer, memberFunction);
  return sender->AddObserver(event, cmd);
}

template <class TObserver>
unsigned long AddListenerPair(
    itk::Object *sender,
    const itk::EventObject &event,
    TObserver *observer,
    void (TObserver::*memberFunction)(itk::Object*, const itk::EventObject &),
    void (TObserver::*constMemberFunction)(const itk::Object*, const itk::EventObject &))
{
  typedef itk::MemberCommand<TObserver> Cmd;
  typename Cmd::Pointer cmd = Cmd::New();
  cmd->SetCallbackFunction(observer, memberFunction);
  cmd->SetCallbackFunction(observer, constMemberFunction);
  return sender->AddObserver(event, cmd);
}


template <class TObserver>
unsigned long AddListenerVTK(
    vtkObject *sender,
    unsigned long event,
    TObserver *observer,
    void (TObserver::*memberFunction)(vtkObject*, unsigned long, void *))
{
  return sender->AddObserver(event, observer, memberFunction);
}



#endif // SNAPEVENTLISTENERCALLBACKS_H
