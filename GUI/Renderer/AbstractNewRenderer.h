#ifndef ABSTRACTNEWRENDERER_H
#define ABSTRACTNEWRENDERER_H

#include <vector>
#include "IRISVectorTypes.h"
#include "AbstractRenderer.h"
#include "itkRGBAPixel.h"
#include "itkImage.h"
#include "SNAPCommon.h"
#include "SNAPAppearanceSettings.h"

/**
 * A dataset (texture or path) optimized for rendering. It inherits from AbstractModel
 * and thus has modification date and can be associated with ITK-SNAP image layers.
 *
 * Internally, it just stores a void* to whatever data structure is used by the render
 * context to represent the texture or path.
 */
template <class Traits>
class NewRenderOptimizedDataset : public AbstractModel
{
public:
  irisITKAbstractObjectMacro(NewRenderOptimizedDataset, AbstractModel)
};


class AbstractNewRenderContext
{
public:
  class TextureTraits {};
  class Path2DTraits {};

  using RGBAPixel = itk::RGBAPixel<unsigned char>;
  using RGBAImage = itk::Image<RGBAPixel>;
  using Path2D = NewRenderOptimizedDataset<Path2DTraits>;
  using Path2DPtr = SmartPtr<Path2D>;
  using Texture = NewRenderOptimizedDataset<TextureTraits>;
  using TexturePtr = SmartPtr<Texture>;

  // Composition modes
  enum CompositionMode {
    SOURCE_OVER, DESTINATION_OVER, MULTIPLY, UNKNOWN
  };

  /** Create a texture from RGBA image. The image should be 32 bit aligned. */
  virtual TexturePtr CreateTexture(RGBAImage *image) = 0;

  /**
   * Colorize a texture. This means that we want to create a texture with
   * the same image as the input, but multiplied by a given color. The render
   * context may use OpenGL to produce this effect using the original texture
   * or it can create a new texture by doing image manipulation
   */
  virtual TexturePtr ColorizeTexture(Texture *texture, const Vector3d &color) = 0;

  /**
   * Draw an image loaded into a texture onto the specified rectangular
   * region.
   */
  virtual void DrawImage(double   x,
                         double   y,
                         double   width,
                         double   height,
                         Texture *texture,
                         bool     bilinear,
                         double   opacity) = 0;

  /**
   * Draw an image loaded into a texture onto the specified rectangular
   * region.
   */
  virtual void DrawImageColorized(double   x,
                                  double   y,
                                  double   width,
                                  double   height,
                                  Texture *texture,
                                  bool     bilinear,
                                  double   opacity,
                                  Vector3d &color) = 0;

  /**
   * Draw a portion of an image loaded into a texture onto the specified
   * rectangular region.
   */
  virtual void       DrawImageRegion(double   x,
                                     double   y,
                                     double   width,
                                     double   height,
                                     double   src_x,
                                     double   src_y,
                                     double   src_width,
                                     double   src_height,
                                     Texture *texture,
                                     bool     bilinear,
                                     double   opacity) = 0;

  // Path creation and use
  virtual Path2DPtr  CreateClosedPolygonPath(std::vector<Vector2d> &path) = 0;
  virtual void DrawPath(void *path) = 0;

  // Pen and brush functions
  virtual void SetPen(const Vector3d &rgb) = 0;
  virtual void SetPen(const OpenGLAppearanceElement &as) = 0;

  // Set brush functions
  virtual void SetBrush(const Vector3d &rgb, double alpha = 1.0) = 0;
  virtual void SetCompositionMode(CompositionMode mode) = 0;
  virtual CompositionMode GetCompositionMode() = 0;

  // Basic drawing functions
  virtual void FillViewport(const Vector3d &rgb) = 0;
  virtual void DrawLine(double x0, double y0, double x1, double y1) = 0;
  virtual void FillRect(double x, double y, double w, double h) = 0;
  virtual void DrawRect(double x, double y, double w, double h) = 0;

  // Text functions
  virtual void DrawText(const std::string &str) = 0;

  // Transform functions
  virtual void PushMatrix() = 0;
  virtual void PopMatrix() = 0;
  virtual void LoadIdentity() = 0;
  virtual void Scale(double sx, double sy) = 0;
  virtual void Translate(double sx, double sy) = 0;

  // Viewport and window control
  virtual void SetViewport(int x, int y, int width, int height) = 0;
  virtual void SetLogicalWindow(int x, int y, int width, int height) = 0;
};

class AbstractNewRenderer : public AbstractRenderer
{
public:
  virtual void Render(AbstractNewRenderContext *context) = 0;
};


#endif // ABSTRACTNEWRENDERER_H
