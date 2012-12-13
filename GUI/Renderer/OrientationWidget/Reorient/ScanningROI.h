#ifndef SCANNING_ROI_H
#define SCANNING_ROI_H

#include <vector>

#include <vtkAssembly.h>
#include <vtkPlaneSource.h>
#include <vtkCaptionActor2D.h>

#include "AxesWidget.h"
#include "Source2ActorPipe.h"
#include "AbstractScannerHelper.h"

struct Pair_PlaneSource_Pipe
{
  vtkSmartPointer < vtkPlaneSource > m_pPlaneSource;
  vtkSmartPointer < Source2ActorPipe > m_pPipePlane;

  Pair_PlaneSource_Pipe();
};

class ScanningROI : public AbstractScannerHelper
{

  double m_arrdbSpacing[3];

  std::vector < Pair_PlaneSource_Pipe > m_arrpPairPSP_Axial;
  std::vector < Pair_PlaneSource_Pipe > m_arrpPairPSP_Coronal;

  vtkSmartPointer < vtkMatrix4x4 > m_pMatrix4x4Directions;

public:
  vtkSmartPointer < AxesWidget > m_pAxesWidget;

private:
  ScanningROI();
public:
  static ScanningROI * New();
  
  void setPlanesNr(int anPlanesNr);
  int getPlanesNr() const;

  void setTranslation(double adbX, double adbY, double adbZ);
  void setTranslation(double aarrdbXYZ[3]);

  void setSpacing(double adbX, double adbY, double adbZ);
  void setSpacing(double aarrdbXYZ[3]);

  void Update();

  virtual void setGraphicScale(double adbGraphicScale);

  vtkSmartPointer < vtkMatrix4x4 > getDirections();
  void setDirections(const vtkSmartPointer < vtkMatrix4x4 > & apMatrix4x4);

  static void changeOrientation3x3(vtkSmartPointer < vtkMatrix4x4 > apvtkMatrix4x4);

};


#endif //SCANNING_ROI_H