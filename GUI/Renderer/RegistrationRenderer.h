#ifndef REGISTRATIONRENDERER_H
#define REGISTRATIONRENDERER_H

#include "SNAPCommon.h"
#include "GenericSliceRenderer.h"

class InteractiveRegistrationModel;

class RegistrationRenderer : public SliceRendererDelegate
{
public:
  irisITKObjectMacro(RegistrationRenderer, SliceRendererDelegate)

  irisSetMacro(Model, InteractiveRegistrationModel *)
  irisGetMacro(Model, InteractiveRegistrationModel *)

  void paintGL();

protected:

  RegistrationRenderer();
  ~RegistrationRenderer();

  InteractiveRegistrationModel *m_Model;
};

#endif // REGISTRATIONRENDERER_H
