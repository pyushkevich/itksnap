#ifndef REGISTRATIONRENDERER_H
#define REGISTRATIONRENDERER_H

#include "SNAPCommon.h"
#include "GenericSliceRenderer.h"
#include "GenericSliceNewRenderer.h"

class InteractiveRegistrationModel;
class OpenGLAppearanceElement;

class RegistrationRenderer : public SliceRendererDelegate
{
public:
  irisITKObjectMacro(RegistrationRenderer, SliceRendererDelegate)

  irisGetSetMacro(Model, InteractiveRegistrationModel *)

  virtual void AddContextItemsToTiledOverlay(
      vtkAbstractContextItem *parent, ImageWrapperBase *) override;

protected:

  RegistrationRenderer();
  ~RegistrationRenderer();

  InteractiveRegistrationModel *m_Model;
};

class RegistrationNewRenderer : public SliceNewRendererDelegate
{
public:
  irisITKObjectMacro(RegistrationNewRenderer, SliceNewRendererDelegate)

  void RenderOverTiledLayer(AbstractNewRenderContext *context,
                            ImageWrapperBase         *base_layer,
                            const SubViewport        &vp) override;

  irisGetSetMacro(Model, InteractiveRegistrationModel *)

protected:
  RegistrationNewRenderer() {}
  virtual ~RegistrationNewRenderer() {}

  void DrawRotationWidget(AbstractNewRenderContext *painter, double beta);
  void DrawGrid(AbstractNewRenderContext *painter);

  InteractiveRegistrationModel *m_Model;
  AbstractNewRenderContext::Path2DPtr m_RotatorPath;
  AbstractNewRenderContext::VertexVector m_Grid;
  Vector2ui m_GridViewportPos, m_GridViewportSize;
};


#endif // REGISTRATIONRENDERER_H
