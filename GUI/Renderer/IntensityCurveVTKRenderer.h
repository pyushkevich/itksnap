#ifndef INTENSITYCURVEVTKRENDERER_H
#define INTENSITYCURVEVTKRENDERER_H

#include "AbstractVTKSceneRenderer.h"
#include "vtkSmartPointer.h"


class vtkActor;
class vtkPropAssembly;
class vtkChartXY;
class vtkFloatArray;
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

  void OnUpdate();

  void UpdatePlotValues();

protected:

  IntensityCurveVTKRenderer();
  virtual ~IntensityCurveVTKRenderer();

  // Rendering stuff
  vtkSmartPointer<vtkChartXY> m_Chart;
  vtkSmartPointer<vtkTable> m_PlotTable;
  vtkSmartPointer<vtkPlot> m_CurvePlot;
  vtkSmartPointer<vtkFloatArray> m_CurveX, m_CurveY;

  // vtkSmartPointer<IntensityCurveControlPointsContextItem> m_ControlPoints;

  vtkSmartPointer<IntensityCurveControlPointsContextItem> m_Controls;

  vtkSmartPointer<vtkTable> m_HistogramTable;
  vtkSmartPointer<vtkPlot> m_HistogramPlot;
  vtkSmartPointer<vtkFloatArray> m_HistogramX, m_HistogramY;

  vtkSmartPointer<HorizontalColorMapContextItem> m_XColorMapItem;
  vtkSmartPointer<VerticalColorMapContextItem> m_YColorMapItem;

  IntensityCurveModel *m_Model;

  enum { CURVE_RESOLUTION = 64 };
};

#endif // INTENSITYCURVEVTKRENDERER_H
