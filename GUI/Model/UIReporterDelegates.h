#ifndef UIREPORTERDELEGATES_H
#define UIREPORTERDELEGATES_H

#include "SNAPCommon.h"
#include "SNAPEvents.h"
#include "itkObject.h"
#include "itkObjectFactory.h"

namespace itk
{
template <class TPixel, unsigned int VDim> class Image;
}

class Registry;

/**
  An interface used by models to request the viewport size from the GUI and
  for the GUI to notify models of changes in the viewport size.
  */
class ViewportSizeReporter : public itk::Object
{
public:
  itkEventMacro(ViewportResizeEvent,IRISEvent)

  /** Child classes are expected to fire this event when viewport resizes */
  FIRES(ViewportResizeEvent)

  virtual bool CanReportSize() = 0;
  virtual Vector2ui GetViewportSize() = 0;

  /** For retina displays, report the ratio of actual GL pixels to logical pixels */
  virtual float GetViewportPixelRatio() = 0;

  /** Get the viewport size in logical units (for retina-like displays) */
  virtual Vector2ui GetLogicalViewportSize() = 0;


protected:
  virtual ~ViewportSizeReporter() {}
};


/**
  This is a progress reporter delegate that allows the SNAP model classes
  to report progress without knowing what GUI toolkit actually implements
  the progress dialog.
  */
class ProgressReporterDelegate
{
public:

  /** Set the progress value between 0 and 1 */
  virtual void SetProgressValue(double) = 0;



};

/**
 This is a delegate that renders text into a bitmap. This is used with
 OpenGL code that needs to draw text
 */
class TextRenderingDelegate
{
public:
  virtual void RenderTextInOpenGL(
      const char *text,
      int x, int y, int w, int h,
      int font_size,
      int align_horiz, int align_vert,
      unsigned char rgba[]) = 0;
};

/**
 This delegate is used to generate an OpenGL texture for icons. Icons
 are identified by a string.
 */
class TextureGenerationDelegate
{
public:
  virtual int GenerateTexture(const char *imageName);
};

/**
 This delegate is used to obtain system-specific information
 */
class SystemInfoDelegate
{
public:
  virtual std::string GetApplicationDirectory() = 0;
  virtual std::string GetApplicationFile() = 0;

  virtual std::string GetApplicationPermanentDataLocation() = 0;

  typedef itk::Image<unsigned char, 2> GrayscaleImage;
  virtual void LoadResourceAsImage2D(std::string tag, GrayscaleImage *image) = 0;
  virtual void LoadResourceAsRegistry(std::string tag, Registry &reg) = 0;
};

#endif // UIREPORTERDELEGATES_H
