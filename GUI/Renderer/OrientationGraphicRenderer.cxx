#include "OrientationGraphicRenderer.h"

#include <vtkSphereSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkProperty.h>
#include <vtkCamera.h>

#include "ScannedHuman.h"
#include "ScanningROI.h"
#include "AxesWidget.h"
#include <PropertyModel.h>

OrientationGraphicRenderer::OrientationGraphicRenderer()
{
  m_Model = NULL;

  //m_Dummy = vtkSmartPointer<vtkSphereSource>::New();
  //m_Dummy->SetCenter(0.0, 0.0, 0.0);
  //m_Dummy->SetRadius(200.00);

//  vtkSmartPointer<vtkPolyDataMapper> mapper =
//      vtkSmartPointer<vtkPolyDataMapper>::New();
//  mapper->SetInputConnection(m_Dummy->GetOutputPort());

//  vtkSmartPointer<vtkActor> actor =
//      vtkSmartPointer<vtkActor>::New();
//  actor->SetMapper(mapper);

//  actor->GetProperty()->SetColor(0, 0, 1);
//  actor->GetProperty()->SetOpacity(1.0);

  //this->m_Renderer->AddActor(actor);

  double dbGraphicScale = 2.0;
  double dbPositionFactor = 2.6;

  m_ReorientProps.Connect2Renderer(m_Renderer);

  vtkCamera * pCamera = m_Renderer->GetActiveCamera();
  double dbPosXYZ = dbGraphicScale * dbPositionFactor;
  pCamera->SetPosition(dbPosXYZ, -dbPosXYZ, dbPosXYZ);
  pCamera->SetViewUp(0.0, -1.0, 0.0);

  this->SetInteractionStyle(TRACKBALL_CAMERA);
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
    vtkSmartPointer < vtkMatrix4x4 > pMatrix4x4 =
        vtkSmartPointer < vtkMatrix4x4 >::New();
    pMatrix4x4->Identity();
    int nI, nJ;
    for(nI = 0; nI < 3; nI++)
      {
      for(nJ = 0; nJ < 3; nJ++)
        {
        (*pMatrix4x4)[nI][nJ] = dm[nI][nJ];
        }
    }
    m_ReorientProps.Update(pMatrix4x4);
    }
  else
    {
    // Hide the slices and arrows
    m_ReorientProps.SetROIVisibility(false);
    }
}

void OrientationGraphicRenderer::Update(const vtkSmartPointer < vtkMatrix4x4 > apMatrix4x4)
{
  m_ReorientProps.Update(apMatrix4x4);

  GetRenderWindow()->Render();
}
