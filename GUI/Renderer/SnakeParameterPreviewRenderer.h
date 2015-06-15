#ifndef SNAKEPARAMETERPREVIEWRENDERER_H
#define SNAKEPARAMETERPREVIEWRENDERER_H

#include "AbstractRenderer.h"
#include "itkRGBAPixel.h"

class SnakeParametersPreviewPipeline;
class SnakeParameterModel;
template <class TPixel> class OpenGLSliceTexture;

class SnakeParameterPreviewRenderer : public AbstractRenderer
{
public:

  irisITKObjectMacro(SnakeParameterPreviewRenderer, AbstractRenderer)

  irisGetMacro(Pipeline, SnakeParametersPreviewPipeline *)

  void SetModel(SnakeParameterModel *model);

  /** An enumeration of different display modes for this widget */
  enum DisplayMode
    {
    PROPAGATION_FORCE=0,CURVATURE_FORCE,ADVECTION_FORCE,TOTAL_FORCE
    };

  /** Set the display mode */
  irisSetMacro(ForceToDisplay,DisplayMode)

  virtual void paintGL();

  virtual void initializeGL();
  virtual void resizeGL(int w, int h, int device_pixel_ratio);

protected:

  // Texture type for drawing speed images
  typedef itk::RGBAPixel<unsigned char> RGBAType;

  /** Model */
  SnakeParameterModel *m_Model;

  /** Preview pipeline logic */
  SnakeParametersPreviewPipeline *m_Pipeline;

  /** A texture object used to store the image */
  OpenGLSliceTexture<RGBAType> *m_Texture;

  /** Which force is being displayed? */
  DisplayMode m_ForceToDisplay;

  int m_ViewportWidth;

  SnakeParameterPreviewRenderer();
  virtual ~SnakeParameterPreviewRenderer();
};

#endif // SNAKEPARAMETERPREVIEWRENDERER_H
