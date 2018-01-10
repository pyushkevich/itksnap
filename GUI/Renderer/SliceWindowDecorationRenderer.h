#ifndef SLICEWINDOWDECORATIONRENDERER_H
#define SLICEWINDOWDECORATIONRENDERER_H

#include "SNAPCommon.h"
#include "GenericSliceRenderer.h"

class GenericSliceRenderer;
class GenericSliceModel;

class SliceWindowDecorationRenderer : public SliceRendererDelegate
{
public:

  irisITKObjectMacro(SliceWindowDecorationRenderer, SliceRendererDelegate)

  virtual void paintGL() ITK_OVERRIDE;

protected:

  void DrawOrientationLabels();

  void DrawRulers();

  void DrawNicknames();

  SliceWindowDecorationRenderer();
  virtual ~SliceWindowDecorationRenderer();

  typedef std::list<std::string> StringList;
  typedef StringList::const_iterator StringListCIter;

  StringList GetDisplayText(ImageWrapperBase *layer);
  std::string CapStringLength(const std::string &str, int max_size);
};

#endif // SLICEWINDOWDECORATIONRENDERER_H
