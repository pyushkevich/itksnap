#include <vtkPolyDataAlgorithm.h>
#include <vtkObjectFactory.h>
#include <vtkActor.h>
#include <vtkPolyDataMapper.h>

#include "PolyDataAlgorithm2ActorPipe.h"

vtkStandardNewMacro(PolyDataAlgorithm2ActorPipe);

PolyDataAlgorithm2ActorPipe::PolyDataAlgorithm2ActorPipe()
{

  m_pvtkPolyDataMapper = vtkSmartPointer < vtkPolyDataMapper >::New();
  m_pvtkActor = vtkSmartPointer < vtkActor >::New();
  m_pvtkActor->SetMapper(m_pvtkPolyDataMapper);

}

void PolyDataAlgorithm2ActorPipe::setSource(vtkSmartPointer < vtkPolyDataAlgorithm > apvtkPolyDataAlgorithm)
{
  m_pvtkPolyDataAlgorithm = apvtkPolyDataAlgorithm;
  m_pvtkPolyDataMapper->SetInputConnection(m_pvtkPolyDataAlgorithm->GetOutputPort());
}

vtkSmartPointer < vtkActor > PolyDataAlgorithm2ActorPipe::getActor()
{
  return(m_pvtkActor);
}

vtkProperty * PolyDataAlgorithm2ActorPipe::getProperty()
{
  vtkActor * pActor = getActor();

  vtkProperty * pProperty = pActor->GetProperty();

  return(pProperty);
}
