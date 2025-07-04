#ifndef INTENSITYCURVEVTKRENDERER_H
#define INTENSITYCURVEVTKRENDERER_H

#include "AbstractVTKSceneRenderer.h"
#include "vtkSmartPointer.h"
#include "LayerHistogramPlotAssembly.h"

class vtkActor;
class vtkPropAssembly;
class vtkChartXY;
class vtkDoubleArray;
class vtkPlot;
class vtkTable;

class IntensityCurveModel;
class IntensityCurveControlPointsContextItem;
class AbstractColorMapContextItem;
class HorizontalColorMapContextItem;
class VerticalColorMapContextItem;

class IntensityCurveVTKRenderer : public AbstractVTKSceneRenderer
{
public:
  irisITKObjectMacro(IntensityCurveVTKRenderer, AbstractVTKSceneRenderer)

  void SetModel(IntensityCurveModel *model);

  void SetRenderWindow(vtkRenderWindow *rwin) override;

  void OnUpdate() override;

  void UpdatePlotValues();

protected:

  IntensityCurveVTKRenderer();
  virtual ~IntensityCurveVTKRenderer();

  // Rendering stuff
  vtkSmartPointer<vtkChartXY> m_Chart;
  vtkSmartPointer<vtkTable> m_PlotTable;
  vtkSmartPointer<vtkPlot> m_CurvePlot;
  vtkSmartPointer<vtkDoubleArray> m_CurveX, m_CurveY;

  // vtkSmartPointer<IntensityCurveControlPointsContextItem> m_ControlPoints;

  vtkSmartPointer<IntensityCurveControlPointsContextItem> m_Controls;

  // Histogram
  LayerHistogramPlotAssembly *m_HistogramAssembly;

  vtkSmartPointer<HorizontalColorMapContextItem> m_XColorMapItem;
  vtkSmartPointer<VerticalColorMapContextItem> m_YColorMapItem;

  IntensityCurveModel *m_Model;

  enum { CURVE_RESOLUTION = 64 };

  void OnCurrentControlPointChangedInScene(vtkObject *, unsigned long, void *);

  virtual void OnDevicePixelRatioChange(int old_ratio, int new_ratio) override;

};

#endif // INTENSITYCURVEVTKRENDERER_H
