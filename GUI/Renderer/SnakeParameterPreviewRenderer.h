#ifndef SNAKEPARAMETERPREVIEWRENDERER_H
#define SNAKEPARAMETERPREVIEWRENDERER_H

#include "AbstractVTKSceneRenderer.h"
#include "itkRGBAPixel.h"

class SnakeParametersPreviewPipeline;
class SnakeParameterModel;
class SnakeParameterContextItem;

class SnakeParameterPreviewRenderer : public AbstractVTKSceneRenderer
{
public:

  irisITKObjectMacro(SnakeParameterPreviewRenderer, AbstractVTKSceneRenderer)

  irisGetMacro(Pipeline, SnakeParametersPreviewPipeline *)

  void SetModel(SnakeParameterModel *model);

  /** An enumeration of different display modes for this widget */
  enum DisplayMode
    {
    PROPAGATION_FORCE=0,CURVATURE_FORCE,ADVECTION_FORCE,TOTAL_FORCE
    };

  /** Set the display mode */
  void SetForceToDisplay(DisplayMode mode);

  void OnUpdate() override;

protected:

  // Texture type for drawing speed images
  typedef itk::RGBAPixel<unsigned char> RGBAType;

  /** Model */
  SnakeParameterModel *m_Model;

  /** Preview pipeline logic */
  SnakeParametersPreviewPipeline *m_Pipeline;

  /** A texture object used to store the image */
  vtkSmartPointer<SnakeParameterContextItem> m_ContextItem;

  /** Which force is being displayed? */
  DisplayMode m_ForceToDisplay;

  int m_ViewportWidth;

  // void OnDevicePixelRatioChange(int old_ratio, int new_ratio) override;

  SnakeParameterPreviewRenderer();
  virtual ~SnakeParameterPreviewRenderer();
};

#endif // SNAKEPARAMETERPREVIEWRENDERER_H
