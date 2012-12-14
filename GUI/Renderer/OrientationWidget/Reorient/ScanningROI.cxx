#include <vtkPolyDataAlgorithm.h>
#include <vtkObjectFactory.h>
#include <vtkProperty.h>
#include <vtkLineSource.h>
#include <vtkPoints.h>
#include <vtkLinearTransform.h>
#include <vtkTransform.h>
#include <vtkArrowSource.h>
#include <vtkCaptionActor2D.h>
#include <vtkTextProperty.h>
#include <vtkProp.h>
#include <vtkProp3D.h>
#include <vtkPolyDataNormals.h>
#include <vtkPlaneSource.h>

#include "ScanningROI.h"


Pair_PlaneSource_Pipe::Pair_PlaneSource_Pipe()
{
  init();
}

void Pair_PlaneSource_Pipe::init()
{
  m_pPlaneSource = vtkSmartPointer < vtkPlaneSource >::New();
  m_pPipePlane = vtkSmartPointer < Source2ActorPipe >::New();
  m_pPipePlane->setSource(m_pPlaneSource);
}

vtkStandardNewMacro(ScanningROI);

ScanningROI::ScanningROI()
{

  m_pMatrix4x4Directions = vtkSmartPointer < vtkMatrix4x4 > ::New();
  m_pMatrix4x4Directions->Identity();

  int nPlanesNr = 5;

  m_arrdbSpacing[0] = 1.0;
  m_arrdbSpacing[1] = 1.0;
  m_arrdbSpacing[2] = 1.0;
  
  setPlanesNr(nPlanesNr);

  m_pAxesWidget = vtkSmartPointer < AxesWidget >::New();
  m_pvtkAssembly->AddPart(m_pAxesWidget->GetAxesActor());

  setGraphicScale(1.0);

}

void ScanningROI::setPlanesNr(int anPlanesNr)
{
  int nI;
  
  int nPlanesNr = m_arrpPairPSP_Axial.size();
  if(nPlanesNr != anPlanesNr)
    {

    for(nI = 0; nI < nPlanesNr; nI++)
	  {
		m_pvtkAssembly->RemovePart(m_arrpPairPSP_Axial[nI].m_pPipePlane->getActor());
	  }
	m_arrpPairPSP_Axial.resize(anPlanesNr);
	for(nI = 0; nI < anPlanesNr; nI++)
	  {
      m_arrpPairPSP_Axial[nI].init();
	  m_pvtkAssembly->AddPart(m_arrpPairPSP_Axial[nI].m_pPipePlane->getActor());
	  }						 
    }
  for(nI = 0; nI < anPlanesNr; nI++)
    {
	vtkSmartPointer < Source2ActorPipe > pPipe = m_arrpPairPSP_Axial[nI].m_pPipePlane;
	vtkProperty * pProperty = pPipe->getProperty();
    //pProperty->SetRepresentationToWireframe();
	pProperty->SetLineWidth(2.0);
	//pProperty->SetLineStipplePattern(0xFF);
	pProperty->SetColor(1.0, 1.0, 1.0);
	//pProperty->BackfaceCullingOn();
	//pProperty->FrontfaceCullingOn();
    pProperty->SetOpacity(0.2);
    }
   
}

int ScanningROI::getPlanesNr() const
{
	return(m_arrpPairPSP_Axial.size());
}

void ScanningROI::setTranslation(double adbX, double adbY, double adbZ)
{
}

void ScanningROI::setTranslation(double aarrdbXYZ[3])
{
}

void ScanningROI::setSpacing(double adbX, double adbY, double adbZ)
{
  m_arrdbSpacing[0] = adbX;
  m_arrdbSpacing[1] = adbY;
  m_arrdbSpacing[2] = adbZ;
}

void ScanningROI::setSpacing(double aarrdbXYZ[3])
{
  setSpacing(aarrdbXYZ[0], aarrdbXYZ[1], aarrdbXYZ[2]);
}

void ScanningROI::Update()
{

  vtkSmartPointer < vtkMatrix4x4 > pMatrix4x4DirectionsAccompanying =
	  vtkSmartPointer < vtkMatrix4x4 >::New();
  pMatrix4x4DirectionsAccompanying->Identity();

  if(m_pMatrix4x4Directions->Determinant() > 0.0)
    {
    m_pAxesWidget->GetAxesActor()->SetPosition(- m_dbGraphicScale / 2.0, -m_dbGraphicScale / 2.0, -m_dbGraphicScale / 2.0);
	m_pAxesWidget->SetLabels("i", "j", "k");
  }
  else
    {
	//m_pAxesWidget->GetAxesActor()->SetPosition(m_dbGraphicScale / 2.0, m_dbGraphicScale / 2.0, m_dbGraphicScale / 2.0);
	
	(*pMatrix4x4DirectionsAccompanying)[0][0] = -1.0;
	(*pMatrix4x4DirectionsAccompanying)[1][1] = 0.0;
	(*pMatrix4x4DirectionsAccompanying)[1][2] = -1.0;
	(*pMatrix4x4DirectionsAccompanying)[2][1] = -1.0;
	(*pMatrix4x4DirectionsAccompanying)[2][2] = 0.0;

	changeOrientation3x3(m_pMatrix4x4Directions);
	m_pAxesWidget->SetLabels("i", "k", "j");
    }

  m_pAxesWidget->GetAxesActor()->SetUserMatrix(pMatrix4x4DirectionsAccompanying);

  //m_pMatrix4x4Directions->DeepCopy(apMatrix4x4);
  //widget->GetProp3D()->SetUserTransform(t);
  vtkSmartPointer < vtkTransform > pTransform = vtkSmartPointer < vtkTransform >::New();
  pTransform->SetMatrix(m_pMatrix4x4Directions);
  vtkLinearTransform *pLinearTransform = m_pvtkAssembly->GetUserTransform();
  if(pLinearTransform == 0)
    m_pvtkAssembly->SetUserTransform(pTransform);
  else
	pLinearTransform->DeepCopy(pTransform);

  m_pAxesWidget->SetLengths(2.0 * m_dbGraphicScale);
  
  int nI;
  int nPlanesNr = getPlanesNr();
  double dbDeltaK = m_dbGraphicScale / ((double)(nPlanesNr - 1));
  for(nI = 0; nI < nPlanesNr; nI++)
    {
    vtkSmartPointer < vtkPlaneSource > pPlane = m_arrpPairPSP_Axial[nI].m_pPlaneSource;
    double arrdbOrigin[4] = {0.0, 0.0, 0.0, 1.0},
      arrdbP1[4] = {0.0, 0.0, 0.0, 1.0},
	  arrdbP2[4] = {0.0, 0.0, 0.0, 1.0};

    arrdbOrigin[0] = - m_dbGraphicScale / 2.0;
	arrdbOrigin[1] = - m_dbGraphicScale / 2.0;
	arrdbOrigin[2] = dbDeltaK * nI - ((int)(((double)nPlanesNr) / 2.0))  * dbDeltaK;

	arrdbP1[0] = m_dbGraphicScale / 2.0;
	arrdbP1[1] = - m_dbGraphicScale / 2.0;
	arrdbP1[2] = arrdbOrigin[2];

	arrdbP2[0] = - m_dbGraphicScale / 2.0;
	arrdbP2[1] = m_dbGraphicScale / 2.0;
	arrdbP2[2] = arrdbOrigin[2];

	pMatrix4x4DirectionsAccompanying->MultiplyDoublePoint(arrdbOrigin);
	pMatrix4x4DirectionsAccompanying->MultiplyDoublePoint(arrdbP2);
	pMatrix4x4DirectionsAccompanying->MultiplyDoublePoint(arrdbP2);

	pPlane->SetOrigin(arrdbOrigin);
	pPlane->SetPoint1(arrdbP1);
	pPlane->SetPoint2(arrdbP2);
    }
//  double dbDeltaI = m_dbGraphicScale / ((double)(nPlanesNr - 1));
//  for(nI = 0; nI < nPlanesNr; nI++)
//    {
//	vtkSmartPointer < vtkPlaneSource > pPlane = m_arrpPairPSP_Coronal[nI].m_pPlaneSource;
//	double arrdbOrigin[3] = {0.0, 0.0, 0.0},
//	  arrdbP1[3] = {0.0, 0.0, 0.0},
//	  arrdbP2[3] = {0.0, 0.0, 0.0};

//	arrdbOrigin[0] = dbDeltaI * nI - ((int)(((double)nPlanesNr) / 2.0))  * dbDeltaI;
//	arrdbOrigin[1] = - m_dbGraphicScale / 2.0;
//	arrdbOrigin[2] = - m_dbGraphicScale / 2.0;

//	arrdbP1[0] = arrdbOrigin[0];
//	arrdbP1[1] = m_dbGraphicScale / 2.0;
//	arrdbP1[2] = - m_dbGraphicScale / 2.0;

//	arrdbP2[0] = arrdbOrigin[0];
//	arrdbP2[1] = - m_dbGraphicScale / 2.0;
//	arrdbP2[2] = m_dbGraphicScale / 2.0;

//	pMatrix4x4DirectionsAccompanying->MultiplyDoublePoint(arrdbOrigin);
//	pMatrix4x4DirectionsAccompanying->MultiplyDoublePoint(arrdbP2);
//	pMatrix4x4DirectionsAccompanying->MultiplyDoublePoint(arrdbP2);

//	pPlane->SetOrigin(arrdbOrigin);
//	pPlane->SetPoint1(arrdbP1);
//	pPlane->SetPoint2(arrdbP2);
//    }
}

void ScanningROI::setGraphicScale(double adbGraphicScale)
{
  AbstractScannerHelper::setGraphicScale(adbGraphicScale);
  Update();
}

vtkSmartPointer < vtkMatrix4x4 > ScanningROI::getDirections()
{
  return(m_pMatrix4x4Directions);
}

void ScanningROI::setDirections(const vtkSmartPointer < vtkMatrix4x4 > & apMatrix4x4)
{
  m_pMatrix4x4Directions->DeepCopy(apMatrix4x4);
}

void ScanningROI::changeOrientation3x3(vtkSmartPointer < vtkMatrix4x4 > apvtkMatrix4x4)
{
  int nI, nJ;
  for(nI = 0; nI < 3; nI++)
    {
    for(nJ = 0; nJ < 3; nJ++)
  	  {
  	  (*apvtkMatrix4x4)[nI][nJ] = - (*apvtkMatrix4x4)[nI][nJ];
  	  }
    }
}
