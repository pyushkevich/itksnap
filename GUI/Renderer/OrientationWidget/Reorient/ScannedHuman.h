#ifndef SCANNED_HUMAN_H
#define SCANNED_HUMAN_H

#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkConeSource.h>
#include <vtkCubeSource.h>
#include <vtkCellArray.h>
#include <vtkTubeFilter.h>
#include <vtkAssembly.h>

#include "PolyDataAlgorithm2ActorPipe.h"
#include "AbstractScannerHelper.h"

class ScannedHuman : public AbstractScannerHelper
{
  vtkSmartPointer < vtkSphereSource > m_pSphereSourceHead;
  vtkSmartPointer < PolyDataAlgorithm2ActorPipe > m_PipeHead;
  
  vtkSmartPointer < vtkSphereSource > m_pSphereSourceRightEye;
  vtkSmartPointer < PolyDataAlgorithm2ActorPipe > m_PipeRightEye;
  
  vtkSmartPointer < vtkSphereSource > m_pSphereSourceLeftEye;
  vtkSmartPointer < PolyDataAlgorithm2ActorPipe > m_PipeLeftEye;
  
  vtkSmartPointer < vtkConeSource > m_pConeSource;
  vtkSmartPointer < PolyDataAlgorithm2ActorPipe > m_PipeCone;

  vtkSmartPointer < vtkPoints > m_pPointsMouth;
  vtkSmartPointer < vtkCellArray > m_pCellArrayMouthLines;
  vtkSmartPointer < vtkTubeFilter > m_pTubeFilterMouth;
  vtkSmartPointer < vtkPolyData > m_pPolyDataMouth;
  vtkSmartPointer < PolyDataAlgorithm2ActorPipe > m_PipeMouth;
  
  vtkSmartPointer < vtkCubeSource > m_pCubeSourceBody;
  vtkSmartPointer < PolyDataAlgorithm2ActorPipe > m_PipeBody;

  ScannedHuman();
public:
  static ScannedHuman * New();
  
  virtual void setGraphicScale(double adbGraphicScale);
};

#endif //SCANNED_HUMAN_H
