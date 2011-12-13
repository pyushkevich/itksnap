#ifndef INTENSITYCURVERENDERER_H
#define INTENSITYCURVERENDERER_H

#include <AbstractModel.h>
class IntensityCurveModel;

class IntensityCurveRenderer : public AbstractModel
{
public:
  irisITKObjectMacro(IntensityCurveRenderer, AbstractModel)


  void SetModel(IntensityCurveModel *model);

  void initializeGL();
  void resizeGL(int w, int h);

  /** Paint the widget. The caller needs to supply the background color */
  void paintGL(int *bkgColor);

protected:

  IntensityCurveRenderer();
  virtual ~IntensityCurveRenderer();

  IntensityCurveModel *m_Model;

  static const unsigned int CURVE_RESOLUTION;


  void DrawCircleWithBorder(double x, double y, double r, double bw,
                            bool select, int w, int h);
};

#endif // INTENSITYCURVERENDERER_H
