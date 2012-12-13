#ifndef REORIENT_H
#define REORIENT_H

#include <vtkSmartPointer.h>
#include <vtkMatrix4x4.h>

#include "AxesWidget.h"

#include "ScannedHuman.h"
#include "ScanningROI.h"

class Reorient
{
  vtkSmartPointer < vtkRenderer > m_pRenderer;
  vtkSmartPointer < vtkRenderWindow > m_pRenWin;
  vtkRenderWindowInteractor * m_pIren;
  
  void updateRenderer();
  
  vtkSmartPointer < AxesWidget > m_pAxesWidgetAbsolute;
  vtkSmartPointer < ScannedHuman > m_pScannedHuman;
  vtkSmartPointer < ScanningROI > m_pScanningROI;

public:

  Reorient();
  ~Reorient();
  
  void setInteractor(vtkRenderWindowInteractor * apIren);
  
  vtkRenderer * GetRenderer();
  vtkRenderWindow * GetRenderWindow();



  void Update(const vtkSmartPointer < vtkMatrix4x4 > apMatrix4x4);

  void SetDirections(const vtkSmartPointer < vtkMatrix4x4 > apDirections);
  void GetDirections(vtkSmartPointer < vtkMatrix4x4 > apDirections) const;
};


#endif //REORIENT_H