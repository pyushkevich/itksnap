#ifndef INTENSITYCURVERENDERER_H
#define INTENSITYCURVERENDERER_H

#include <AbstractRenderer.h>
class IntensityCurveModel;

class IntensityCurveRenderer : public AbstractRenderer
{
public:
  irisITKObjectMacro(IntensityCurveRenderer, AbstractRenderer)


  void SetModel(IntensityCurveModel *model);

  void initializeGL();
  void resizeGL(int w, int h);

  /** Paint the widget. The caller needs to supply the background color */
  void paintGL();

  void OnUpdate();

  // Set the background color to paint with
  irisGetSetMacro(Background, Vector3ui)

protected:

  IntensityCurveRenderer();
  virtual ~IntensityCurveRenderer();

  IntensityCurveModel *m_Model;

  static const unsigned int CURVE_RESOLUTION;

  Vector3ui m_Background;

  void DrawCircleWithBorder(double x, double y, double r, double bw,
                            bool select, int w, int h);
};

#endif // INTENSITYCURVERENDERER_H
