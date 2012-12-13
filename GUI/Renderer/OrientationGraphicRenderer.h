#ifndef ORIENTATIONGRAPHICRENDERER_H
#define ORIENTATIONGRAPHICRENDERER_H

#include "vtkMatrix4x4.h"
#include "AbstractVTKRenderer.h"
#include "vtkSmartPointer.h"
#include "PropertyModel.h"

class AxesWidget;
class ScannedHuman;
class ScanningROI;

class vtkSphereSource;

class OrientationGraphicRenderer : public AbstractVTKRenderer
{
public:

  typedef vnl_matrix<double> DirectionMatrix;
  typedef AbstractPropertyModel<DirectionMatrix> DirectionMatrixModel;

  irisITKObjectMacro(OrientationGraphicRenderer, AbstractVTKRenderer)

  void SetModel(DirectionMatrixModel *model);

  void OnUpdate();

  void Update(const vtkSmartPointer < vtkMatrix4x4 > apMatrix4x4);

  void SetDirections(const vtkSmartPointer < vtkMatrix4x4 > apDirections);
  void GetDirections(vtkSmartPointer < vtkMatrix4x4 > apDirections) const;

protected:

  OrientationGraphicRenderer();
  virtual ~OrientationGraphicRenderer() {}

  DirectionMatrixModel *m_Model;

  vtkSmartPointer < AxesWidget > m_pAxesWidgetAbsolute;
  vtkSmartPointer < ScannedHuman > m_pScannedHuman;
  vtkSmartPointer < ScanningROI > m_pScanningROI;
};

#endif // ORIENTATIONGRAPHICRENDERER_H
