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
  m_pAxesWidgetAbsolute = vtkSmartPointer < AxesWidget >::New();
  m_pAxesWidgetAbsolute->SetLengths(dbGraphicScale * 1.5);
  this->m_Renderer->AddActor(m_pAxesWidgetAbsolute->GetAxesActor());

  m_pScannedHuman = vtkSmartPointer < ScannedHuman >::New();
  m_pScannedHuman->setGraphicScale(1.0);
  this->m_Renderer->AddActor(m_pScannedHuman->getAssembly());

  m_pScanningROI = vtkSmartPointer < ScanningROI >::New();
  m_pScanningROI->setGraphicScale(dbGraphicScale);
  this->m_Renderer->AddActor(m_pScanningROI->getAssembly());

  m_Renderer->AddActor(m_pScanningROI->m_pAxesWidget->GetAxesActor()->GetXAxisCaptionActor2D());
  m_Renderer->AddActor(m_pScanningROI->m_pAxesWidget->GetAxesActor()->GetYAxisCaptionActor2D());
  m_Renderer->AddActor(m_pScanningROI->m_pAxesWidget->GetAxesActor()->GetZAxisCaptionActor2D());

  vtkCamera * pCamera = m_Renderer->GetActiveCamera();
  pCamera->SetPosition(dbGraphicScale * 5.0, -dbGraphicScale * 5.0, dbGraphicScale * 5.0);
  pCamera->SetViewUp(0.0, -1.0, 0.0);

  //vtkRenderWindow * pvtkRenderWindow = GetRenderWindow();
  //m_pIren = vtkSmartPointer < vtkRenderWindowInteractor >::New();
  //pvtkRenderWindow->SetInteractor(m_pIren);

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
    Update(pMatrix4x4);
    }
  else
    {
    // Hide the slices and arrows
    }
}

void OrientationGraphicRenderer::Update(const vtkSmartPointer < vtkMatrix4x4 > apMatrix4x4)
{
  SetDirections(apMatrix4x4);

  m_pScanningROI->Update();

  GetRenderWindow()->Render();
}

void OrientationGraphicRenderer::SetDirections(const vtkSmartPointer < vtkMatrix4x4 > apDirections)
{
    m_pScanningROI->setDirections(apDirections);
}

void OrientationGraphicRenderer::GetDirections(vtkSmartPointer < vtkMatrix4x4 > apDirections) const
{
  vtkSmartPointer < vtkMatrix4x4 > pDirections =
      m_pScanningROI->getDirections();
  apDirections->DeepCopy(pDirections);
}
