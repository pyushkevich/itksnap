#include <vtkSmartPointer.h>
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
#include <vtkActor.h>
#include <vtkPolyDataNormals.h>
#include <vtkPlaneSource.h>
#include <vtkAxesActor.h>
#include <vtkAssembly.h>
#include <vtkTubeFilter.h>
#include <vtkCellArray.h>
#include <vtkLine.h>
#include <vtkLineSource.h>

#include "PolyDataAlgorithm2ActorPipe.h"
#include "ScanningROI.h"
#include "AxesWidget.h"

Pairs_Plane_Pipe::Pairs_Plane_Pipe()
{
  init();
}

void Pairs_Plane_Pipe::init(int anGridResolution)
{
  m_p_PlaneSource = vtkSmartPointer < vtkPlaneSource >::New();
  m_p_PlaneSource_Pipe = vtkSmartPointer < PolyDataAlgorithm2ActorPipe >::New();
  m_p_PlaneSource_Pipe->setSource(m_p_PlaneSource);

  m_p_TubeFilter_WireFrame = vtkSmartPointer < vtkTubeFilter >::New();
  m_p_TubeFilter_WireFrame_Pipe = vtkSmartPointer < PolyDataAlgorithm2ActorPipe >::New();
  m_p_TubeFilter_WireFrame_Pipe->setSource(m_p_TubeFilter_WireFrame);

  m_arrp_TubeFilter_PlanarGrid.resize(anGridResolution);
  m_arrp_TubeFilter_PlanarGrid_Pipe.resize(anGridResolution);
  int nI;
  for(nI = 0; nI < anGridResolution; nI++)
    {
    m_arrp_TubeFilter_PlanarGrid[nI] = vtkSmartPointer < vtkTubeFilter >::New();
    m_arrp_TubeFilter_PlanarGrid_Pipe[nI] = vtkSmartPointer < PolyDataAlgorithm2ActorPipe >::New();
    m_arrp_TubeFilter_PlanarGrid_Pipe[nI]->setSource(m_arrp_TubeFilter_PlanarGrid[nI]);
    }
}

void Pairs_Plane_Pipe::SetVisibility(int anVisibility)
{
  m_p_PlaneSource_Pipe->getActor()->SetVisibility(anVisibility);

  m_p_TubeFilter_WireFrame_Pipe->getActor()->SetVisibility(anVisibility);

  for(size_t nI = 0; nI < m_arrp_TubeFilter_PlanarGrid_Pipe.size(); nI++)
    {
    m_arrp_TubeFilter_PlanarGrid_Pipe[nI]->getActor()->SetVisibility(anVisibility);
    }
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

  int nI, nJ;
  
  int nPlanesNr = (int) m_arrpPairsPP_Axial.size();
  if(nPlanesNr != anPlanesNr)
    {

    for(nI = 0; nI < nPlanesNr; nI++)
	  {
      Pairs_Plane_Pipe & ppp = m_arrpPairsPP_Axial[nI];
      m_pvtkAssembly->RemovePart(ppp.m_p_PlaneSource_Pipe->getActor());
      m_pvtkAssembly->RemovePart(ppp.m_p_TubeFilter_WireFrame_Pipe->getActor());
      for(nJ = 0; nJ < nPlanesNr; nJ++)
        {
        m_pvtkAssembly->RemovePart(ppp.m_arrp_TubeFilter_PlanarGrid_Pipe[nJ]->getActor());
        }
      }
    m_arrpPairsPP_Axial.resize(anPlanesNr);
	for(nI = 0; nI < anPlanesNr; nI++)
	  {
      Pairs_Plane_Pipe & ppp = m_arrpPairsPP_Axial[nI];
      ppp.init(anPlanesNr);
      m_pvtkAssembly->AddPart(ppp.m_p_PlaneSource_Pipe->getActor());
      m_pvtkAssembly->AddPart(ppp.m_p_TubeFilter_WireFrame_Pipe->getActor());
      for(nJ = 0; nJ < anPlanesNr; nJ++)
        {
        m_pvtkAssembly->AddPart(ppp.m_arrp_TubeFilter_PlanarGrid_Pipe[nJ]->getActor());
        }
	  }						 
    }
  for(nI = 0; nI < anPlanesNr; nI++)
    {
    Pairs_Plane_Pipe & ppp = m_arrpPairsPP_Axial[nI];
    vtkSmartPointer < PolyDataAlgorithm2ActorPipe > pPipe = ppp.m_p_PlaneSource_Pipe;
	vtkProperty * pProperty = pPipe->getProperty();
    pProperty->SetColor(1.0, 1.0, 1.0);
    pProperty->SetOpacity(0.2);

    pPipe = ppp.m_p_TubeFilter_WireFrame_Pipe;
    pProperty = pPipe->getProperty();
    pProperty->SetColor(1.0, 1.0, 1.0);

    for(nJ = 0; nJ < anPlanesNr; nJ++)
      {
      pPipe = ppp.m_arrp_TubeFilter_PlanarGrid_Pipe[nJ];
      pProperty = pPipe->getProperty();
      pProperty->SetColor(0.8, 0.8, 0.8);
      pProperty->SetOpacity(0.2);
      }
    }
   
}

int ScanningROI::getPlanesNr() const
{
  return (int) m_arrpPairsPP_Axial.size();
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
    m_pAxesWidget->SetColors(AxesWidget::m_arrdbColRed, AxesWidget::m_arrdbColGreen, AxesWidget::m_arrdbColBlue);
    }
  else
    {
	//m_pAxesWidget->GetAxesActor()->SetPosition(m_dbGraphicScale / 2.0, m_dbGraphicScale / 2.0, m_dbGraphicScale / 2.0);
	
    pMatrix4x4DirectionsAccompanying->SetElement(0, 0, -1.0);
    pMatrix4x4DirectionsAccompanying->SetElement(1, 1, 0.0);
    pMatrix4x4DirectionsAccompanying->SetElement(1, 2, -1.0);
    pMatrix4x4DirectionsAccompanying->SetElement(2, 1, -1.0);
    pMatrix4x4DirectionsAccompanying->SetElement(2, 2, 0.0);

	changeOrientation3x3(m_pMatrix4x4Directions);
	m_pAxesWidget->SetLabels("i", "k", "j");
    m_pAxesWidget->SetColors(AxesWidget::m_arrdbColRed, AxesWidget::m_arrdbColBlue, AxesWidget::m_arrdbColGreen);
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
  double dbDelta = m_dbGraphicScale / ((double)(nPlanesNr - 1));
  for(nI = 0; nI < nPlanesNr; nI++)
    {
    //First, create the transparent planes
    double arrdbOrigin[4] = {0.0, 0.0, 0.0, 1.0},
      arrdbP1[4] = {0.0, 0.0, 0.0, 1.0},
	  arrdbP2[4] = {0.0, 0.0, 0.0, 1.0};

    arrdbOrigin[0] = - m_dbGraphicScale / 2.0;
	arrdbOrigin[1] = - m_dbGraphicScale / 2.0;
    arrdbOrigin[2] = dbDelta * nI - ((int)(((double)nPlanesNr) / 2.0))  * dbDelta;

	arrdbP1[0] = m_dbGraphicScale / 2.0;
	arrdbP1[1] = - m_dbGraphicScale / 2.0;
	arrdbP1[2] = arrdbOrigin[2];

	arrdbP2[0] = - m_dbGraphicScale / 2.0;
	arrdbP2[1] = m_dbGraphicScale / 2.0;
	arrdbP2[2] = arrdbOrigin[2];

    //pMatrix4x4DirectionsAccompanying->MultiplyDoublePoint(arrdbOrigin);
    //pMatrix4x4DirectionsAccompanying->MultiplyDoublePoint(arrdbP2);
    //pMatrix4x4DirectionsAccompanying->MultiplyDoublePoint(arrdbP2);

    Pairs_Plane_Pipe & ppp = m_arrpPairsPP_Axial[nI];
    vtkSmartPointer < vtkPlaneSource > pPlaneSource = ppp.m_p_PlaneSource;
    pPlaneSource->SetOrigin(arrdbOrigin);
    pPlaneSource->SetPoint1(arrdbP1);
    pPlaneSource->SetPoint2(arrdbP2);

    //Second, insert the TubeFilter edge wires
    double arrdbP3[4] = {0.0, 0.0, 0.0, 1.0};
    arrdbP3[0] = arrdbP1[0];
    arrdbP3[1] = arrdbP2[1];
    arrdbP3[2] = arrdbOrigin[2];

    vtkSmartPointer<vtkPoints> pPoints = vtkSmartPointer<vtkPoints>::New();
    pPoints->InsertPoint(0, arrdbOrigin);
    pPoints->InsertPoint(1, arrdbP1);

    //Yes, the order is correct, this point is the 4th one
    //in defining order on top of the Plane sorce!
    pPoints->InsertPoint(2, arrdbP3);

    pPoints->InsertPoint(3, arrdbP2);
    pPoints->InsertPoint(4, arrdbOrigin);

    vtkSmartPointer<vtkCellArray> pLines = vtkSmartPointer<vtkCellArray>::New();
    pLines->InsertNextCell(5);
    int nJ;
    for (nJ = 0; nJ < 5; nJ++)
      {
      pLines->InsertCellPoint(nJ);
      }

    vtkSmartPointer<vtkPolyData> pPolyData = vtkSmartPointer<vtkPolyData>::New();
    pPolyData->SetPoints(pPoints);
    pPolyData->SetLines(pLines);

    vtkSmartPointer < vtkTubeFilter > pTubeFilter = ppp.m_p_TubeFilter_WireFrame;
    pTubeFilter->SetInputData(pPolyData);
    pTubeFilter->SetNumberOfSides(100);
    pTubeFilter->SetRadius(0.01 * m_dbGraphicScale);

    //All right, now let's set the grid
    for(nJ = 0; nJ < nPlanesNr; nJ++)
      {
      vtkSmartPointer< vtkLineSource > pLineSource =
        vtkSmartPointer< vtkLineSource >::New();
      pLineSource->SetPoint1(arrdbOrigin[0] + dbDelta * nJ, arrdbOrigin[1], arrdbOrigin[2]);
      pLineSource->SetPoint2(arrdbOrigin[0] + dbDelta * nJ, arrdbP2[1], arrdbOrigin[2]);
      vtkSmartPointer < vtkTubeFilter > pTubeFilter =
        ppp.m_arrp_TubeFilter_PlanarGrid[nJ];
      pTubeFilter->SetInputConnection(pLineSource->GetOutputPort());
      pTubeFilter->SetNumberOfSides(100);
      pTubeFilter->SetRadius(0.005 * m_dbGraphicScale);
      }
    }

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
      apvtkMatrix4x4->SetElement(nI, nJ, apvtkMatrix4x4->GetElement(nI, nJ));
  	  }
    }
}

void ScanningROI::SetVisibility(int anVisibility)
{
  for(size_t nI = 0; nI < m_arrpPairsPP_Axial.size(); nI++)
  {
  m_arrpPairsPP_Axial[nI].SetVisibility(anVisibility);
  }
  m_pAxesWidget->SetVisibility(anVisibility);
}
