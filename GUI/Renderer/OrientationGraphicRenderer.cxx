#include "OrientationGraphicRenderer.h"

#include <vtkSphereSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkProperty.h>

#include <ReorientImageModel.h>

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

  m_pAxesWidgetAbsolute = vtkSmartPointer < AxesWidget >::New();
  m_pAxesWidgetAbsolute->SetLengths(3.0);
  this->m_Renderer->AddActor(m_pAxesWidgetAbsolute->GetAxesActor());

  m_pScannedHuman = vtkSmartPointer < ScannedHuman >::New();
  m_pScannedHuman->setGraphicScale(1.0);
  this->m_Renderer->AddActor(m_pScannedHuman->getAssembly());

  m_pScanningROI = vtkSmartPointer < ScanningROI >::New();
  m_pScanningROI->setGraphicScale(2.0);
  this->m_Renderer->AddActor(m_pScanningROI->getAssembly());

  this->m_Renderer->AddActor(m_pScanningROI->m_pAxesWidget->GetAxesActor()->GetXAxisCaptionActor2D());
  this->m_Renderer->AddActor(m_pScanningROI->m_pAxesWidget->GetAxesActor()->GetYAxisCaptionActor2D());
  this->m_Renderer->AddActor(m_pScanningROI->m_pAxesWidget->GetAxesActor()->GetZAxisCaptionActor2D());

}



void OrientationGraphicRenderer::SetModel(ReorientImageModel *model)
{
  this->m_Model = model;

  // Rebroadcast the relevant events from the model in order for the
  // widget that uses this renderer to cause an update
  Rebroadcast(m_Model, ModelUpdateEvent(), ModelUpdateEvent());
}

void OrientationGraphicRenderer::OnUpdate()
{
  // ???
}

void OrientationGraphicRenderer::Update(const vtkSmartPointer < vtkMatrix4x4 > apMatrix4x4)
{
  SetDirections(apMatrix4x4);

  m_pScanningROI->Update();

  //Tavi m_pRenWin->Render();
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
