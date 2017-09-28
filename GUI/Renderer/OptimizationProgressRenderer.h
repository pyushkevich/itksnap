#ifndef OPTIMIZATIONPROGRESSRENDERER_H
#define OPTIMIZATIONPROGRESSRENDERER_H

#include "AbstractVTKSceneRenderer.h"
#include "vtkSmartPointer.h"

class vtkActor;
class vtkPropAssembly;
class vtkChartXY;
class vtkFloatArray;
class vtkPlot;
class vtkTable;

class RegistrationModel;

class OptimizationProgressRenderer : public AbstractVTKSceneRenderer
{
public:

  irisITKObjectMacro(OptimizationProgressRenderer, AbstractVTKSceneRenderer)

  void SetModel(RegistrationModel *model);

  itkGetMacro(PyramidLevel, int)
  itkSetMacro(PyramidLevel, int)

  itkGetMacro(PyramidZoom, int)
  itkSetMacro(PyramidZoom, int)

  void OnUpdate() ITK_OVERRIDE;

  void UpdatePlotValues();

protected:

  OptimizationProgressRenderer();

  virtual ~OptimizationProgressRenderer() {}

  RegistrationModel *m_Model;

  // Rendering stuff
  vtkSmartPointer<vtkChartXY> m_Chart;
  vtkSmartPointer<vtkTable> m_PlotTable;
  vtkSmartPointer<vtkPlot> m_Plot;
  vtkSmartPointer<vtkFloatArray> m_DataX, m_DataY;

  // Range of values
  double m_MinValue, m_MaxValue;

  // Pyramid level that this plots
  int m_PyramidLevel, m_PyramidZoom;
};

#endif // OPTIMIZATIONPROGRESSRENDERER_H
