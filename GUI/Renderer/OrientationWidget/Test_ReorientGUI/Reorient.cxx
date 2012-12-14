#include "qvtkwidget.h"

#include "vtkCamera.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkWindowToImageFilter.h"
#include "vtkBMPWriter.h"
#include "vtkTextProperty.h"
#include "vtkCubeAxesActor2D.h"

#include "ScanningROI.h"
#include "Reorient.h"

Reorient::Reorient()
{

  m_pRenderer = vtkSmartPointer < vtkRenderer >::New();
  //m_pRenderer->TwoSidedLightingOn();
  m_pRenWin = vtkSmartPointer < vtkRenderWindow >::New();
  m_pRenWin->SetSize(200, 500);
  m_pRenWin->AddRenderer(m_pRenderer);

  Connect2Renderer(m_pRenderer);

  vtkCamera * pCamera = m_pRenderer->GetActiveCamera();
  pCamera->SetPosition(10.0, -10.0, 10.0);
  pCamera->SetViewUp(0.0, -1.0, 0.0);

  Connect2Renderer(m_pRenderer);
}

Reorient::~Reorient()
{
}

void Reorient::setInteractor(vtkRenderWindowInteractor * apIren)
{
  m_pIren = apIren;
}

void Reorient::updateRenderer() 
{

  m_pRenderer->ResetCamera();
  m_pRenWin->Render();

}

vtkRenderer * Reorient::GetRenderer()
{
  return(m_pRenderer);
}

vtkRenderWindow * Reorient::GetRenderWindow()
{
  return(m_pRenWin);
}

void Reorient::Update(const vtkSmartPointer < vtkMatrix4x4 > apMatrix4x4)
{
  ReorientProps::Update(apMatrix4x4);

  m_pRenWin->Render();
}
