#ifndef THRESHOLDSETTINGSRENDERER_H
#define THRESHOLDSETTINGSRENDERER_H

#include "AbstractVTKSceneRenderer.h"
#include "vtkSmartPointer.h"

class vtkActor;
class vtkPropAssembly;
class vtkChartXY;
class vtkFloatArray;
class vtkPlot;
class vtkTable;

class SnakeWizardModel;
class LayerHistogramPlotAssembly;

class ThresholdSettingsRenderer : public AbstractVTKSceneRenderer
{
public:
  irisITKObjectMacro(ThresholdSettingsRenderer, AbstractVTKSceneRenderer)

  void SetModel(SnakeWizardModel *model);

  void OnUpdate();

  void UpdatePlotValues();
protected:

  ThresholdSettingsRenderer();
  virtual ~ThresholdSettingsRenderer();

  SnakeWizardModel *m_Model;

  // Number of data points
  static const unsigned int NUM_POINTS;

  // Rendering stuff
  vtkSmartPointer<vtkChartXY> m_Chart;
  vtkSmartPointer<vtkTable> m_PlotTable;
  vtkSmartPointer<vtkPlot> m_Plot;
  vtkSmartPointer<vtkFloatArray> m_DataX, m_DataY;

  // Histogram rendering
  LayerHistogramPlotAssembly *m_HistogramAssembly;

  void OnDevicePixelRatioChange(int old_ratio, int new_ratio);
};

#endif // THRESHOLDSETTINGSRENDERER_H
