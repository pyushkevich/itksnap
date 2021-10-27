#ifndef ABSTRACTRENDERER_H
#define ABSTRACTRENDERER_H

#include "AbstractModel.h"

class vtkImageData;

/**
 * @brief The RendererPlatformSupport class
 * This class holds various methods that renderers require from the underlying
 * GUI platform. These include drawing text, for example. Before any renderers
 * can be used, the renderer platform support must be assigned to the abstract
 * renderer using the static method AbstractRenderer::SetPlatformSupport()
 */
class AbstractRendererPlatformSupport
{
public:

  enum FontType { SANS, SERIF, TYPEWRITER };
  enum HAlign { LEFT=-1, HCENTER=0, RIGHT=1 };
  enum VAlign { BOTTOM=-1, VCENTER=0, TOP=1 };

  struct FontInfo {
    FontType type;
    int pixel_size;
    bool bold;
  };

  virtual FontInfo MakeFont(int pixel_size, FontType type = SANS, bool bold=false);

  virtual void RenderTextIntoVTKImage(
      const char *text, vtkImageData *target,
      FontInfo font, int align_horiz, int align_vert,
      const Vector3d &rgbf, double alpha = 1.0) = 0;

  virtual int MeasureTextWidth(const char *text, FontInfo font) = 0;
};


/**
  A parent class for ITK-SNAP renderers. A renderer implements all the
  drawing code independently of the widget system (Qt).
  */
class AbstractRenderer : public AbstractModel
{
public:

  static void SetPlatformSupport(AbstractRendererPlatformSupport *support)
    { m_PlatformSupport = support; }

  static AbstractRendererPlatformSupport* GetPlatformSupport()
    { return m_PlatformSupport; }

  virtual void SaveAsPNG(std::string filename);

protected:
  AbstractRenderer();
  virtual ~AbstractRenderer() {}

  static AbstractRendererPlatformSupport *m_PlatformSupport;
};

#endif // ABSTRACTRENDERER_H
