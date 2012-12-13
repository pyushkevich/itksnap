#include "OrientationGraphicRenderer.h"

#include <vtkSphereSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkProperty.h>

#include <PropertyModel.h>

OrientationGraphicRenderer::OrientationGraphicRenderer()
{
  m_Model = NULL;

  m_Dummy = vtkSmartPointer<vtkSphereSource>::New();
  m_Dummy->SetCenter(0.0, 0.0, 0.0);
  m_Dummy->SetRadius(200.00);

  vtkSmartPointer<vtkPolyDataMapper> mapper =
      vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(m_Dummy->GetOutputPort());

  vtkSmartPointer<vtkActor> actor =
      vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  actor->GetProperty()->SetColor(0, 0, 1);
  actor->GetProperty()->SetOpacity(1.0);

  this->m_Renderer->AddActor(actor);
}



void OrientationGraphicRenderer::SetModel(DirectionMatrixModel *model)
{
  this->m_Model = model;

  // Rebroadcast the relevant events from the model in order for the
  // widget that uses this renderer to cause an update
  Rebroadcast(m_Model, ValueChangedEvent(), ModelUpdateEvent());
}

void OrientationGraphicRenderer::OnUpdate()
{
  // Get the current direction matrix from the model ...
  DirectionMatrix dm;
  if(m_Model->GetValueAndDomain(dm, NULL))
    {
    // Set the transform on the slices and arrows ...
    }
  else
    {
    // Hide the slices and arrows
    }
}


