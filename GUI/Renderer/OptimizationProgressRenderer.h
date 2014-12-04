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

class ImageIOWizardModel;

class OptimizationProgressRenderer : public AbstractVTKSceneRenderer
{
public:

  irisITKObjectMacro(OptimizationProgressRenderer, AbstractVTKSceneRenderer)

  void SetModel(ImageIOWizardModel *model);

  void OnUpdate();

  void UpdatePlotValues();

protected:

  OptimizationProgressRenderer();

  virtual ~OptimizationProgressRenderer() {}

  ImageIOWizardModel *m_Model;

  // Rendering stuff
  vtkSmartPointer<vtkChartXY> m_Chart;
  vtkSmartPointer<vtkTable> m_PlotTable;
  vtkSmartPointer<vtkPlot> m_Plot;
  vtkSmartPointer<vtkFloatArray> m_DataX, m_DataY;

  // Range of values
  double m_MinValue, m_MaxValue;
};

#endif // OPTIMIZATIONPROGRESSRENDERER_H
