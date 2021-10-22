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

  irisGetSetMacro(Model, InteractiveRegistrationModel *)

  virtual void AddContextItemsToTiledOverlay(
      vtkAbstractContextItem *parent, ImageWrapperBase *) override;

  void paintGL() ITK_OVERRIDE;

protected:

  RegistrationRenderer();
  ~RegistrationRenderer();

  InteractiveRegistrationModel *m_Model;
private:
  void DrawRotationWidget(const OpenGLAppearanceElement *ae);
};

#endif // REGISTRATIONRENDERER_H
