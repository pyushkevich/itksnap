#ifndef SCANNING_ROI_H
#define SCANNING_ROI_H

#include <vector>

template <class T> class vtkSmartPointer;
class vtkAssembly;
class vtkPlaneSource;
class vtkCaptionActor2D;
class vtkTubeFilter;

class AxesWidget;
class PolyDataAlgorithm2ActorPipe;

#include "AbstractScannerHelper.h"

struct Pairs_Plane_Pipe
{
  vtkSmartPointer < vtkPlaneSource > m_p_PlaneSource;
  vtkSmartPointer < PolyDataAlgorithm2ActorPipe > m_p_PlaneSource_Pipe;

  vtkSmartPointer < vtkTubeFilter > m_p_TubeFilter;
  vtkSmartPointer < PolyDataAlgorithm2ActorPipe > m_p_TubeFilter_Pipe;

  Pairs_Plane_Pipe();

  void init();
};

class ScanningROI : public AbstractScannerHelper
{

  double m_arrdbSpacing[3];

  std::vector < Pairs_Plane_Pipe > m_arrpPairsPP_Axial;

  vtkSmartPointer < vtkMatrix4x4 > m_pMatrix4x4Directions;

public:
  vtkSmartPointer < AxesWidget > m_pAxesWidget;

private:
  ScanningROI();
public:
  static ScanningROI * New();
  
  void setPlanesNr(int anPlanesNr);
  int getPlanesNr() const;

  void setSpacing(double adbX, double adbY, double adbZ);
  void setSpacing(double aarrdbXYZ[3]);

  void Update();

  virtual void setGraphicScale(double adbGraphicScale);

  vtkSmartPointer < vtkMatrix4x4 > getDirections();
  void setDirections(const vtkSmartPointer < vtkMatrix4x4 > & apMatrix4x4);

  static void changeOrientation3x3(vtkSmartPointer < vtkMatrix4x4 > apvtkMatrix4x4);

};

#endif //SCANNING_ROI_H
