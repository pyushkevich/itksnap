#include "ActorPool.h"
#include "vtkActor.h"
#include "vtkPolyDataMapper.h"

vtkActor*
ActorPool::
GetNewActor()
{
  if (m_SpareActors.size() <= 0)
    CreateNewActors(5u);

  vtkActor* ret = m_SpareActors.top();
  m_SpareActors.pop();
  return ret;
}

void
ActorPool::
Recycle(vtkActor* actor)
{
  actor->RemoveAllObservers();
  m_SpareActors.push(actor);
}

void
ActorPool::
RecycleAll()
{
  for (auto it = m_ActorMap.begin(); it != m_ActorMap.end(); ++it)
    {
    Recycle(it->second);
    }

  m_ActorMap.clear();
}

void
ActorPool::
CreateNewActors(unsigned int n)
{
  while (n-- > 0)
    {
    vtkNew<vtkActor> actor;
    vtkNew<vtkPolyDataMapper> mapper;
    actor->SetMapper(mapper);
    m_ActorStorage.push(actor);
    m_SpareActors.push(actor);
    }
}

void
ActorPool::
Print(std::ostream &os) const
{
  os << "Actor Pool: " << this << std::endl;
  os << "Total # actors: " << m_ActorStorage.size() << std::endl;
  os << "# spare actors: " << m_SpareActors.size() << std::endl;
  os << "# actors in the map: " << m_ActorMap.size() << std::endl;
}
