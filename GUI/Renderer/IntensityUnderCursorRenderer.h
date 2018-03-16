#ifndef INTENSITYUNDERCURSORRENDERER_H
#define INTENSITYUNDERCURSORRENDERER_H

#include "AbstractVTKSceneRenderer.h"
class ImageInfoModel;
class vtkChartXY;
class vtkTable;
class vtkPlot;
class vtkDoubleArray;
class vtkIdTypeArray;

/**
 * @brief Renderer used to plot the intensity profile for multi-component images
 */
class IntensityUnderCursorRenderer : public AbstractVTKSceneRenderer
{
public:
  irisITKObjectMacro(IntensityUnderCursorRenderer, AbstractVTKSceneRenderer)

  void SetModel(ImageInfoModel *model);

  void OnUpdate() ITK_OVERRIDE;

  void UpdatePlotValues();

  virtual void paintGL() ITK_OVERRIDE;

  virtual void OnDevicePixelRatioChange(int old_ratio, int new_ratio) ITK_OVERRIDE;

protected:
  IntensityUnderCursorRenderer();
  virtual ~IntensityUnderCursorRenderer();

  // Rendering stuff
  vtkSmartPointer<vtkChartXY> m_Chart;
  vtkSmartPointer<vtkTable> m_PlotTable;
  vtkSmartPointer<vtkPlot> m_CurvePlot;
  vtkSmartPointer<vtkDoubleArray> m_CurveX, m_CurveY;
  vtkSmartPointer<vtkIdTypeArray> m_Selection;

  // Model
  ImageInfoModel *m_Model;
};

#endif // INTENSITYUNDERCURSORRENDERER_H
