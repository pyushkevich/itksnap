#ifndef SOURCE_2_ACTOR_PIPE_H
#define SOURCE_2_ACTOR_PIPE_H

#include <vtkObject.h>
#include <vtkSmartPointer.h>
#include <vtkPolyDataAlgorithm.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>

class Source2ActorPipe : public vtkObject
{
  vtkSmartPointer < vtkPolyDataAlgorithm > m_pvtkPolyDataAlgorithm;
  vtkSmartPointer < vtkPolyDataMapper > m_pvtkPolyDataMapper;
  vtkSmartPointer < vtkActor > m_pvtkActor;

  Source2ActorPipe();
public:

  static Source2ActorPipe *New();
  
  void setSource(vtkSmartPointer < vtkPolyDataAlgorithm > apvtkPolyDataAlgorithm);
  
  vtkSmartPointer < vtkActor > getActor();
  vtkProperty * getProperty();
};


#endif //SOURCE_2_ACTOR_PIPE_H