#ifndef REORIENT_H
#define REORIENT_H

#include <vtkSmartPointer.h>
#include <vtkMatrix4x4.h>

//#include "AxesWidget.h"

//#include "ScannedHuman.h"
//#include "ScanningROI.h"
#include "ReorientProps.h"

class Reorient : public ReorientProps
{
  vtkSmartPointer < vtkRenderer > m_pRenderer;
  vtkSmartPointer < vtkRenderWindow > m_pRenWin;
  vtkRenderWindowInteractor * m_pIren;
  
  void updateRenderer();
  
public:

  Reorient();
  ~Reorient();
  
  void setInteractor(vtkRenderWindowInteractor * apIren);
  
  vtkRenderer * GetRenderer();
  vtkRenderWindow * GetRenderWindow();

  virtual void Update(const vtkSmartPointer < vtkMatrix4x4 > apMatrix4x4);
};


#endif //REORIENT_H
