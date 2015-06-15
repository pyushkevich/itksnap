#ifndef COLORMAPRENDERER_H
#define COLORMAPRENDERER_H

#include "AbstractRenderer.h"

class ColorMapModel;

class ColorMapRenderer : public AbstractRenderer
{
public:
  irisITKObjectMacro(ColorMapRenderer, AbstractRenderer)


  void SetModel(ColorMapModel *model);

  void resizeGL(int w, int h, int device_pixel_ratio);
  void paintGL();

protected:

  ColorMapRenderer();
  virtual ~ColorMapRenderer();

  ColorMapModel *m_Model;


  unsigned int m_TextureId;
  unsigned int m_Width, m_Height;
};



#endif // COLORMAPRENDERER_H
