#include "vtkCamera.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkWindowToImageFilter.h"
#include "vtkBMPWriter.h"
#include "vtkTextProperty.h"
#include "vtkCaptionActor2D.h"
#include "vtkMath.h"

#include "ScanningROI.h"
#include "ReorientProps.h"

bool ReorientProps::isMatrixValid(const vtkSmartPointer < vtkMatrix4x4 > apMatrix4x4)
{
  const double dbError = 0.01;
  double dbDeterminant = apMatrix4x4->Determinant();
  if(fabs(fabs(dbDeterminant) -  1.0) > dbError)
  {
    return(false);
  }

  int nI, nJ;
  for(nI = 0; nI < 3; nI++)
    {
    double * pdbLine1 = apMatrix4x4->Element[nI];
    for(nJ = nI + 1; nJ < 3; nJ++)
      {
      double * pdbLine2 = apMatrix4x4->Element[nJ];
      //check orthogonality
      double dbDotProd = vtkMath::Dot(pdbLine1, pdbLine2);
      if(fabs(dbDotProd) > dbError)
      {
        return(false);
      }
      //check normality
      double dbNorm = vtkMath::Norm(pdbLine1);
      if(fabs(dbNorm - 1.0) > dbError)
      {
          return(false);
      }
      }
    }

  return(true);
}

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
  if(isMatrixValid(apMatrix4x4))
    {
    m_pScanningROI->SetVisibility(1);
    SetDirections(apMatrix4x4);
    }
  else
    m_pScanningROI->SetVisibility(0);
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

  vtkSmartPointer < vtkAxesActor > pAxesActor = m_pScanningROI->m_pAxesWidget->GetAxesActor();
  apRenderer->AddActor(pAxesActor->GetXAxisCaptionActor2D());
  apRenderer->AddActor(pAxesActor->GetYAxisCaptionActor2D());
  apRenderer->AddActor(pAxesActor->GetZAxisCaptionActor2D());
}

void ReorientProps::SetROIVisibility(int anVisibility)
{
  m_pScanningROI->SetVisibility(anVisibility);
}
