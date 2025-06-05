#ifndef REGISTRATIONRENDERER_H
#define REGISTRATIONRENDERER_H

#include "SNAPCommon.h"
#include "GenericSliceRenderer.h"

class InteractiveRegistrationModel;
class OpenGLAppearanceElement;

class RegistrationRenderer : public SliceRendererDelegate
{
public:
  irisITKObjectMacro(RegistrationRenderer, SliceRendererDelegate)

  void RenderOverTiledLayer(AbstractRenderContext *context,
                            ImageWrapperBase         *base_layer,
                            const SubViewport        &vp) override;

  irisGetSetMacro(Model, InteractiveRegistrationModel *)

protected:
  RegistrationRenderer() {}
  virtual ~RegistrationRenderer() {}

  void DrawRotationWidget(AbstractRenderContext *painter, double beta);
  void DrawGrid(AbstractRenderContext *painter);

  InteractiveRegistrationModel *m_Model;
  AbstractRenderContext::Path2DPtr m_RotatorPath;
  AbstractRenderContext::VertexVector m_Grid;
  Vector2ui m_GridViewportPos, m_GridViewportSize;
};


#endif // REGISTRATIONRENDERER_H
