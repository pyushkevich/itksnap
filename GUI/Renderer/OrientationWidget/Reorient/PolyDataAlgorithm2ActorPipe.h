#ifndef SOURCE_2_ACTOR_PIPE_H
#define SOURCE_2_ACTOR_PIPE_H

#include "vtkSmartPointer.h"
class vtkObject;
class vtkPolyDataAlgorithm;
class vtkPolyDataMapper;
class vtkActor;
class vtkProperty;

class PolyDataAlgorithm2ActorPipe : public vtkObject
{
  vtkSmartPointer < vtkPolyDataAlgorithm > m_pvtkPolyDataAlgorithm;
  vtkSmartPointer < vtkPolyDataMapper > m_pvtkPolyDataMapper;
  vtkSmartPointer < vtkActor > m_pvtkActor;

  PolyDataAlgorithm2ActorPipe();
public:

  static PolyDataAlgorithm2ActorPipe *New();
  
  void setSource(vtkSmartPointer < vtkPolyDataAlgorithm > apvtkPolyDataAlgorithm);
  
  vtkSmartPointer < vtkActor > getActor();
  vtkProperty * getProperty();
};

#endif //SOURCE_2_ACTOR_PIPE_H
