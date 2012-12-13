#include <vtkPolyDataAlgorithm.h>
#include <vtkObjectFactory.h>

#include "Source2ActorPipe.h"

vtkStandardNewMacro(Source2ActorPipe);

Source2ActorPipe::Source2ActorPipe()
{

  m_pvtkPolyDataMapper = vtkSmartPointer < vtkPolyDataMapper >::New();
  m_pvtkActor = vtkSmartPointer < vtkActor >::New();
  m_pvtkActor->SetMapper(m_pvtkPolyDataMapper);

}

void Source2ActorPipe::setSource(vtkSmartPointer < vtkPolyDataAlgorithm > apvtkPolyDataAlgorithm)
{
  m_pvtkPolyDataAlgorithm = apvtkPolyDataAlgorithm;
  m_pvtkPolyDataMapper->SetInput(m_pvtkPolyDataAlgorithm->GetOutput());
}

vtkSmartPointer < vtkActor > Source2ActorPipe::getActor()
{
  return(m_pvtkActor);
}

vtkProperty * Source2ActorPipe::getProperty() 
{
  vtkActor * pActor = getActor();

  vtkProperty * pProperty = pActor->GetProperty();

  return(pProperty);
}