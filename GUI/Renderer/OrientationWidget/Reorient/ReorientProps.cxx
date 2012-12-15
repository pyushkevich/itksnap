#include "vtkCamera.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkWindowToImageFilter.h"
#include "vtkBMPWriter.h"
#include "vtkTextProperty.h"
#include "vtkCubeAxesActor2D.h"

#include "ScanningROI.h"
#include "ReorientProps.h"

ReorientProps::ReorientProps()
{
  //m_pAxesWidgetAbsolute = vtkSmartPointer < AxesWidget >::New();
  //m_pAxesWidgetAbsolute->SetLengths(3.0);

  m_pScannedHuman = vtkSmartPointer < ScannedHuman >::New();
  m_pScannedHuman->setGraphicScale(1.0);

  m_pScanningROI = vtkSmartPointer < ScanningROI >::New();
  m_pScanningROI->setGraphicScale(2.0);
}

ReorientProps::~ReorientProps()
{
}

void ReorientProps::Update(const vtkSmartPointer < vtkMatrix4x4 > apMatrix4x4)
{
  SetDirections(apMatrix4x4);

  m_pScanningROI->Update();
}

void ReorientProps::SetDirections(const vtkSmartPointer < vtkMatrix4x4 > apDirections)
{
	m_pScanningROI->setDirections(apDirections);
}

void ReorientProps::GetDirections(vtkSmartPointer < vtkMatrix4x4 > apDirections) const
{
  vtkSmartPointer < vtkMatrix4x4 > pDirections =
	  m_pScanningROI->getDirections();
  apDirections->DeepCopy(pDirections);
}

void ReorientProps::Connect2Renderer(vtkRenderer * apRenderer)
{
  //apRenderer->AddActor(m_pAxesWidgetAbsolute->GetAxesActor());
  apRenderer->AddActor(m_pScannedHuman->getAssembly());
  apRenderer->AddActor(m_pScanningROI->getAssembly());

  apRenderer->AddActor(m_pScanningROI->m_pAxesWidget->GetAxesActor()->GetXAxisCaptionActor2D());
  apRenderer->AddActor(m_pScanningROI->m_pAxesWidget->GetAxesActor()->GetYAxisCaptionActor2D());
  apRenderer->AddActor(m_pScanningROI->m_pAxesWidget->GetAxesActor()->GetZAxisCaptionActor2D());
}
