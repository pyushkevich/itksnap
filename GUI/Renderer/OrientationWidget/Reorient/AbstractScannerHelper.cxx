#include <vtkObjectFactory.h>
#include <vtkAssembly.h>

#include "AbstractScannerHelper.h"

AbstractScannerHelper::AbstractScannerHelper()
{

  //setGraphicScale(1.0);
  
  m_pvtkAssembly = vtkSmartPointer < vtkAssembly >::New();
  
}

AbstractScannerHelper::~AbstractScannerHelper()
{
}

void AbstractScannerHelper::setGraphicScale(double adbGraphicScale)
{

  m_dbGraphicScale = adbGraphicScale;

}

vtkSmartPointer < vtkAssembly > AbstractScannerHelper::getAssembly()
{
  return(m_pvtkAssembly);
}
