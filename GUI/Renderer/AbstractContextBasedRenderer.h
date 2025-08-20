#ifndef ABSTRACTNEWRENDERER_H
#define ABSTRACTNEWRENDERER_H

#include <vector>
#include "IRISVectorTypes.h"
#include "AbstractRenderer.h"
#include "itkRGBAPixel.h"
#include "itkImage.h"
#include "SNAPCommon.h"
#include "SNAPAppearanceSettings.h"

class vtkPolyData;
class vtkUnsignedCharArray;

/**
 * A dataset (texture or path) optimized for rendering. It inherits from AbstractModel
 * and thus has modification date and can be associated with ITK-SNAP image layers.
 *
 * Internally, it just stores a void* to whatever data structure is used by the render
 * context to represent the texture or path.
 */
template <class Traits>
class RenderOptimizedDataset : public AbstractModel
{
public:
  irisITKAbstractObjectMacro(RenderOptimizedDataset, AbstractModel)
};


class AbstractRenderContext
{
public:
  class TextureTraits {};
  class Path2DTraits {};
  class ContourSet2DTraits {};

  using RGBAPixel = itk::RGBAPixel<unsigned char>;
  using RGBAImage = itk::Image<RGBAPixel>;
  using Path2D = RenderOptimizedDataset<Path2DTraits>;
  using Path2DPtr = SmartPtr<Path2D>;
  using Texture = RenderOptimizedDataset<TextureTraits>;
  using TexturePtr = SmartPtr<Texture>;
  using ContourSet2D = RenderOptimizedDataset<ContourSet2DTraits>;
  using ContourSet2DPtr = SmartPtr<ContourSet2D>;
  using FontInfo = AbstractRendererPlatformSupport::FontInfo;

  using VertexVector = std::vector<Vector2d>;

  // Composition modes
  enum CompositionMode {
    SOURCE_OVER, DESTINATION_OVER, MULTIPLY, UNKNOWN
  };

  // How color is applied
  enum PolyDataColorMode {
    SOLID_COLOR, POINT_DATA, CELL_DATA
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
  virtual Path2DPtr CreatePath() = 0;
  virtual void AddPolygonSegmentToPath(Path2D *path, const VertexVector &segment, bool closed) = 0;
  virtual void BuildPath(Path2D *path) = 0;
  virtual void DrawPath(Path2D *path) = 0;

  // Contour rendering using VTK
  virtual ContourSet2DPtr CreateContourSet() = 0;
  virtual void AddContoursToSet(ContourSet2D *cset, vtkPolyData *pd, PolyDataColorMode mode = SOLID_COLOR, vtkUnsignedCharArray *rgbvec = nullptr) = 0;
  virtual void BuildContourSet(ContourSet2D *cset) = 0;
  virtual void DrawContourSet(ContourSet2D *cset) = 0;

  // Pen and brush functions
  virtual void SetPenColor(const Vector3d &rgb) = 0;
  virtual void SetPenColor(const Vector3d &rgb, double alpha) = 0;
  virtual void SetPenOpacity(double alpha) = 0;
  virtual void SetPenWidth(double width) = 0;
  virtual void SetPenLineType(int type) = 0;
  virtual void SetPenAppearance(const OpenGLAppearanceElement &as) = 0;

  // Set brush functions
  virtual void SetBrush(const Vector3d &rgb, double alpha = 1.0) = 0;
  virtual void SetCompositionMode(CompositionMode mode) = 0;
  virtual CompositionMode GetCompositionMode() = 0;

  // Basic drawing functions
  virtual void FillViewport(const Vector3d &rgb) = 0;
  virtual void DrawLine(double x0, double y0, double x1, double y1) = 0;
  virtual void DrawLines(const VertexVector &vertex_pairs) = 0;
  virtual void DrawPolyLine(const VertexVector &polyline) = 0;
  virtual void FillRect(double x, double y, double w, double h) = 0;
  virtual void DrawRect(double x, double y, double w, double h) = 0;
  virtual void DrawPoint(double x, double y) = 0;
  virtual void DrawEllipse(double x, double y, double rx, double ry) = 0;

  // Text functions
  virtual void SetFont(const FontInfo &fi) = 0;
  virtual int TextWidth(const std::string &str) = 0;
  virtual void DrawText(const std::string &str) = 0;
  virtual void DrawText(const std::string &text,
                        double             x,
                        double             y,
                        double             w,
                        double             h,
                        int                halign,
                        int                valign) = 0;

  // Transform functions
  virtual void PushMatrix() = 0;
  virtual void PopMatrix() = 0;
  virtual void LoadIdentity() = 0;
  virtual void Scale(double sx, double sy) = 0;
  virtual void Translate(double sx, double sy) = 0;
  virtual void Rotate(double angle_in_degrees) = 0;

  // Viewport and window control
  virtual void SetViewport(int x, int y, int width, int height) = 0;
  virtual void SetLogicalWindow(int x, int y, int width, int height) = 0;
  virtual double GetDevicePixelRatio() = 0;

  // Transform support functions
  virtual Vector2d MapScreenOffsetToWorldOffset(const Vector2d &offset,
                                                bool            physical_units = false) = 0;
};

class AbstractContextBasedRenderer : public AbstractRenderer
{
public:
  virtual void Render(AbstractRenderContext *context) = 0;
};


#endif // ABSTRACTNEWRENDERER_H
