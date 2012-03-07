#ifndef COLORMAPRENDERER_H
#define COLORMAPRENDERER_H

#include "AbstractModel.h"

class ColorMapModel;

class ColorMapRenderer : public AbstractModel
{
public:
  irisITKObjectMacro(ColorMapRenderer, AbstractModel)


  void SetModel(ColorMapModel *model);

  void initializeGL();
  void resizeGL(int w, int h);
  void paintGL();

protected:

  ColorMapRenderer();
  virtual ~ColorMapRenderer();

  ColorMapModel *m_Model;


  unsigned int m_TextureId;
  unsigned int m_Width, m_Height;
};



#endif // COLORMAPRENDERER_H
