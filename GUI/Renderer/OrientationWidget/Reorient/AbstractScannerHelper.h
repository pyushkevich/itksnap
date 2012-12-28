#ifndef ABSTRACT_SCANNER_HELPER_H
#define ABSTRACT_SCANNER_HELPER_H

#include "vtkSmartPointer.h"
class vtkObject;
class vtkAssembly;

class PolyDataAlgorithm2ActorPipe;

class AbstractScannerHelper : public vtkObject
{
protected:

  vtkSmartPointer < vtkAssembly > m_pvtkAssembly;
  double m_dbGraphicScale;
  
  AbstractScannerHelper();
  virtual ~AbstractScannerHelper() = 0;

public:
  static AbstractScannerHelper * New();
  
  virtual void setGraphicScale(double adbGraphicScale) = 0;
  vtkSmartPointer < vtkAssembly > getAssembly();
};


#endif //ABSTRACT_SCANNER_HELPER_H
