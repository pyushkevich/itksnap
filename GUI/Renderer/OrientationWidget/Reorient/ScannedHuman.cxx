#include <vtkPolyDataAlgorithm.h>
#include <vtkObjectFactory.h>
#include <vtkProperty.h>
#include <vtkActor.h>
#include <vtkLineSource.h>
#include <vtkPoints.h>

#include "ScannedHuman.h"

vtkStandardNewMacro(ScannedHuman);

ScannedHuman::ScannedHuman()
{
  m_pSphereSourceHead = vtkSmartPointer < vtkSphereSource >::New();
  m_pSphereSourceRightEye = vtkSmartPointer < vtkSphereSource >::New();
  m_pSphereSourceLeftEye = vtkSmartPointer < vtkSphereSource >::New();
  m_pConeSource = vtkSmartPointer < vtkConeSource >::New();

  m_pPolyDataMouth = vtkSmartPointer < vtkPolyData >::New();
  m_pPointsMouth = vtkSmartPointer < vtkPoints >::New();
  m_pPolyDataMouth->SetPoints(m_pPointsMouth);
  m_pCellArrayMouthLines = vtkSmartPointer < vtkCellArray >::New();
  m_pPolyDataMouth->SetLines(m_pCellArrayMouthLines);

  m_pTubeFilterMouth = vtkSmartPointer < vtkTubeFilter >::New();
  m_pTubeFilterMouth->SetInputData(m_pPolyDataMouth);
  
  m_pCubeSourceBody = vtkSmartPointer < vtkCubeSource >::New();
    
  m_PipeHead = vtkSmartPointer < PolyDataAlgorithm2ActorPipe >::New();
  m_PipeRightEye = vtkSmartPointer < PolyDataAlgorithm2ActorPipe >::New();
  m_PipeLeftEye = vtkSmartPointer < PolyDataAlgorithm2ActorPipe >::New();
  m_PipeCone = vtkSmartPointer < PolyDataAlgorithm2ActorPipe >::New();
  m_PipeMouth = vtkSmartPointer < PolyDataAlgorithm2ActorPipe >::New();
  m_PipeBody = vtkSmartPointer < PolyDataAlgorithm2ActorPipe >::New();
  
  m_PipeHead->setSource(m_pSphereSourceHead);
  m_PipeRightEye->setSource(m_pSphereSourceRightEye);
  m_PipeLeftEye->setSource(m_pSphereSourceLeftEye);
  m_PipeCone->setSource(m_pConeSource);
  m_PipeMouth->setSource(m_pTubeFilterMouth);
  m_PipeBody->setSource(m_pCubeSourceBody);

  m_pvtkAssembly->AddPart(m_PipeHead->getActor());
  m_pvtkAssembly->AddPart(m_PipeRightEye->getActor());
  m_pvtkAssembly->AddPart(m_PipeLeftEye->getActor());
  m_pvtkAssembly->AddPart(m_PipeCone->getActor());
  m_pvtkAssembly->AddPart(m_PipeMouth->getActor());
  m_pvtkAssembly->AddPart(m_PipeBody->getActor());

  setGraphicScale(1.0);

}

void ScannedHuman::setGraphicScale(double adbGraphicScale)
{
  AbstractScannerHelper::setGraphicScale(adbGraphicScale);

  double dbL = adbGraphicScale;
  
  vtkSmartPointer < vtkActor > pActor = m_PipeHead->getActor();
  vtkProperty * pProperty = pActor->GetProperty();
  pProperty->SetColor(0.0, 0.0, 1.0);
  //pProperty->SetDiffuseColor(0.0, 0.0, 1.0);
  //pProperty->SetDiffuse(1.0);
  pProperty->SetSpecularColor(0.0, 0.0, 1.0);
  pProperty->SetSpecular(1.0);
  pActor->SetPosition(0.0, 0.0, dbL * 1.5);
  m_pSphereSourceHead->SetThetaResolution(100);
  m_pSphereSourceHead->SetPhiResolution(100);

  pActor = m_PipeRightEye->getActor();
  pProperty = pActor->GetProperty();
  pProperty->SetColor(1.0, 1.0, 1.0);
  pActor->SetPosition(- dbL / 5, - dbL / 2.85, dbL * 1.7);
  m_pSphereSourceRightEye->SetRadius(dbL/8.0);
  m_pSphereSourceRightEye->SetThetaResolution(100);
  m_pSphereSourceRightEye->SetPhiResolution(100);

  pActor = m_PipeLeftEye->getActor();
  pProperty = pActor->GetProperty();
  pProperty->SetColor(1.0, 1.0, 1.0);
  pActor->SetPosition(dbL / 5, - dbL / 2.85, dbL * 1.7);
  m_pSphereSourceLeftEye->SetRadius(dbL/8.0);
  m_pSphereSourceLeftEye->SetThetaResolution(100);
  m_pSphereSourceLeftEye->SetPhiResolution(100);
  
  pActor = m_PipeCone->getActor();
  pProperty = pActor->GetProperty();
  pProperty->SetColor(1.0, 1.0, 1.0);
  double arrdbConeCenter[3];
  arrdbConeCenter[0] = 0.0;
  arrdbConeCenter[1] = - dbL * 0.6;
  arrdbConeCenter[2] = dbL * 1.5;
  pActor->SetPosition(arrdbConeCenter);
  m_pConeSource->SetHeight(dbL * 0.25);
  m_pConeSource->SetRadius(dbL * 0.1);
  m_pConeSource->SetDirection(0.0, - 1.0, 0.0);
  m_pConeSource->SetResolution(100);
  
  pActor = m_PipeMouth->getActor();
  pProperty = pActor->GetProperty();
  pProperty->SetColor(1.0, 1.0, 1.0);
  m_pTubeFilterMouth->SetRadius(0.05*dbL);
  m_pTubeFilterMouth->SetNumberOfSides(100);
  int nPointsNr = 11;
  m_pPointsMouth->SetNumberOfPoints(nPointsNr);
  int nI;
  for(nI = 0; nI < nPointsNr; nI++)
  {
	double dbAlpha = 5.0 * ((double)(nI - 5)) / 180.0;
	m_pPointsMouth->SetPoint (nI,
		arrdbConeCenter[0] + sin(dbAlpha) * dbL,
		arrdbConeCenter[1] - cos(dbAlpha) * dbL + dbL * 1.15,
		arrdbConeCenter[2] - dbL * 0.2);
  }
  m_pCellArrayMouthLines->InsertNextCell(nPointsNr);
  for(nI = 0; nI < nPointsNr; nI++)
    m_pCellArrayMouthLines->InsertCellPoint(nI);

  pActor = m_PipeBody->getActor();
  pProperty = pActor->GetProperty();
  pProperty->SetColor(0.0, 0.0, 1.0);
  //pProperty->SetDiffuseColor(0.0, 0.0, 1.0);
  //pProperty->SetDiffuse(1.0);
  pProperty->SetSpecularColor(0.0, 0.0, 1.0);
  pProperty->SetSpecular(0.5);
  m_pCubeSourceBody->SetYLength(0.75 * dbL);
  m_pCubeSourceBody->SetZLength(2 * dbL);

}
