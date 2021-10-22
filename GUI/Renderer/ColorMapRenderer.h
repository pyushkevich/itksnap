#ifndef COLORMAPRENDERER_H
#define COLORMAPRENDERER_H

#include "AbstractVTKSceneRenderer.h"

class ColorMapModel;
class vtkRenderWindow;
class ColorMapContextItem;

class ColorMapRenderer : public AbstractVTKSceneRenderer
{
public:
  irisITKObjectMacro(ColorMapRenderer, AbstractVTKSceneRenderer)


  void SetModel(ColorMapModel *model);
  void OnUpdate() override;

  virtual void paintGL() override;
protected:

  ColorMapRenderer();
  virtual ~ColorMapRenderer();

  ColorMapModel *m_Model;

  vtkSmartPointer<ColorMapContextItem> m_ContextItem;
  unsigned int m_Width, m_Height;
};



#endif // COLORMAPRENDERER_H
