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

  vtkSmartPointer<vtkTable> m_ControlTable;
  vtkSmartPointer<vtkPlot> m_ControlPlot;
  vtkSmartPointer<vtkFloatArray> m_ControlX, m_ControlY;


  IntensityCurveModel *m_Model;

  enum { CURVE_RESOLUTION = 64 };
};

#endif // INTENSITYCURVEVTKRENDERER_H
