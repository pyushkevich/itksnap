#ifndef ACTORPOOL_H
#define ACTORPOOL_H

#include "SNAPCommon.h"
#include "itkObject.h"
#include "itkObjectFactory.h"
#include "vtkSmartPointer.h"
#include <stack>

class vtkActor;
class vtkPolyDataMapper;

class ActorPool : public itk::Object
{
public:
  // Standard ITK macros
  irisITKObjectMacro(ActorPool, itk::Object)

  typedef std::map<LabelType, vtkActor*> ActorMap;
  typedef std::stack<vtkActor*> SpareActorStack;
  typedef std::stack<vtkSmartPointer<vtkActor>> ActorStorage;

  /** Get the pointer to the actor map
   *  Returning a pointer to avoid copying
   */
  ActorMap *GetActorMap()
  { return &m_ActorMap; }

  /** Get a spare actor from the pool */
  vtkActor *GetNewActor();

  /** Recycle an actor to the pool */
  void Recycle(vtkActor* actor);

  /** Recycle all actors */
  void RecycleAll();

  /** Print status */
  void Print(std::ostream &os) const;

protected:
  ActorPool() {}
  virtual ~ActorPool() {}

  // Create batch of new actors and add it to the reserve
  void CreateNewActors(unsigned int n);

  // Maps id to active actors
  ActorMap m_ActorMap;

  // Stores spare actors
  SpareActorStack m_SpareActors;

  // Actual Storage of all actors
  ActorStorage m_ActorStorage;
};
#endif // ACTORPOOL_H
