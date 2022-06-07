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

protected:

  RegistrationRenderer();
  ~RegistrationRenderer();

  InteractiveRegistrationModel *m_Model;
};

#endif // REGISTRATIONRENDERER_H
