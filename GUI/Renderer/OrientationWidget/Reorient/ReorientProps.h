#ifndef REORIENT_PROPS_H
#define REORIENT_PROPS_H

#include <vtkSmartPointer.h>
#include <vtkMatrix4x4.h>

#include "AxesWidget.h"

#include "ScannedHuman.h"
#include "ScanningROI.h"

class ReorientProps
{
  //vtkSmartPointer < AxesWidget > m_pAxesWidgetAbsolute;
  vtkSmartPointer < ScannedHuman > m_pScannedHuman;
  vtkSmartPointer < ScanningROI > m_pScanningROI;

  static bool isMatrixValid(const vtkSmartPointer < vtkMatrix4x4 > apMatrix4x4);

public:

  ReorientProps();
  ~ReorientProps();
  
  void Update(const vtkSmartPointer < vtkMatrix4x4 > apMatrix4x4);

  void SetDirections(const vtkSmartPointer < vtkMatrix4x4 > apDirections);
  void GetDirections(vtkSmartPointer < vtkMatrix4x4 > apDirections) const;

  virtual void Connect2Renderer(vtkRenderer * apRenderer);

  void SetROIVisibility(int anVisibility);
};


#endif //REORIENT_PROPS_H
