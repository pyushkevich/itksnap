#include "AbstractRenderer.h"

// The platform support initializes to NULL
AbstractRendererPlatformSupport *AbstractRenderer::m_PlatformSupport = NULL;

AbstractRenderer::AbstractRenderer()
{
}

void AbstractRenderer::SaveAsPNG(std::string filename)
{

}


AbstractRendererPlatformSupport::FontInfo
AbstractRendererPlatformSupport::MakeFont(
    int pixel_size, AbstractRendererPlatformSupport::FontType type, bool bold)
{
  FontInfo fi = { type, pixel_size, bold };
  return fi;
}
