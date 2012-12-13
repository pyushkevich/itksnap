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

  m_pAxesWidgetAbsolute = vtkSmartPointer < AxesWidget >::New();
  m_pAxesWidgetAbsolute->SetLengths(3.0);
  m_pRenderer->AddActor(m_pAxesWidgetAbsolute->GetAxesActor());

  m_pScannedHuman = vtkSmartPointer < ScannedHuman >::New();
  m_pScannedHuman->setGraphicScale(1.0);
  m_pRenderer->AddActor(m_pScannedHuman->getAssembly());

  m_pScanningROI = vtkSmartPointer < ScanningROI >::New();
  m_pScanningROI->setGraphicScale(2.0);
  m_pRenderer->AddActor(m_pScanningROI->getAssembly());

  m_pRenderer->AddActor(m_pScanningROI->m_pAxesWidget->GetAxesActor()->GetXAxisCaptionActor2D());
  m_pRenderer->AddActor(m_pScanningROI->m_pAxesWidget->GetAxesActor()->GetYAxisCaptionActor2D());
  m_pRenderer->AddActor(m_pScanningROI->m_pAxesWidget->GetAxesActor()->GetZAxisCaptionActor2D());

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
  SetDirections(apMatrix4x4);

  m_pScanningROI->Update();

  m_pRenWin->Render();
}

void Reorient::SetDirections(const vtkSmartPointer < vtkMatrix4x4 > apDirections)
{
	m_pScanningROI->setDirections(apDirections);
}

void Reorient::GetDirections(vtkSmartPointer < vtkMatrix4x4 > apDirections) const
{
  vtkSmartPointer < vtkMatrix4x4 > pDirections =
	  m_pScanningROI->getDirections();
  apDirections->DeepCopy(pDirections);
}