#ifndef ORIENTATIONGRAPHICRENDERER_H
#define ORIENTATIONGRAPHICRENDERER_H

#include "vtkMatrix4x4.h"
#include "AbstractVTKRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkRenderWindowInteractor.h"
#include "OrientationWidget/ReorientGUI/AxesWidget.h"
#include "OrientationWidget/ReorientGUI/ScannedHuman.h"
#include "OrientationWidget/ReorientGUI/ScanningROI.h"

class vtkSphereSource;

class ReorientImageModel;

class OrientationGraphicRenderer : public AbstractVTKRenderer
{
public:
  irisITKObjectMacro(OrientationGraphicRenderer, AbstractVTKRenderer)

  void SetModel(ReorientImageModel *model);

  void OnUpdate();

  void Update(const vtkSmartPointer < vtkMatrix4x4 > apMatrix4x4);

  void SetDirections(const vtkSmartPointer < vtkMatrix4x4 > apDirections);
  void GetDirections(vtkSmartPointer < vtkMatrix4x4 > apDirections) const;

protected:

  OrientationGraphicRenderer();
  virtual ~OrientationGraphicRenderer() {}

  ReorientImageModel *m_Model;

  // A sphere to render
  //vtkSmartPointer<vtkSphereSource> m_Dummy;

  vtkSmartPointer < AxesWidget > m_pAxesWidgetAbsolute;
  vtkSmartPointer < ScannedHuman > m_pScannedHuman;
  vtkSmartPointer < ScanningROI > m_pScanningROI;

  //vtkSmartPointer < vtkRenderWindowInteractor > m_pIren;
};

#endif // ORIENTATIONGRAPHICRENDERER_H
