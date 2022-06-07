#ifndef GENERICSLICECONTEXTITEM_H
#define GENERICSLICECONTEXTITEM_H

#include <vtkContextItem.h>
#include <vtkSmartPointer.h>
#include "SNAPCommon.h"
#include "AbstractRenderer.h"

class vtkPen;
class vtkBrush;
class GenericSliceModel;
class OpenGLAppearanceElement;
class ImageWrapperBase;

/**
 * @brief Parent class for items rendered in slice views using the VTK
 * Context/Scene API
 */
class GenericSliceContextItem : public vtkContextItem
{
public:
  vtkTypeMacro(GenericSliceContextItem, vtkContextItem)

  irisSetMacro(Model, GenericSliceModel *);
  irisGetMacro(Model, GenericSliceModel *);

  /** Apply appearance settings to a pen */
  void ApplyAppearanceSettingsToPen(
      vtkContext2D *painter, const OpenGLAppearanceElement *as);

  /** Apply font appearance settings */
  void ApplyAppearanceSettingsToFont(
      vtkContext2D *painter, const OpenGLAppearanceElement *as);

  /** Draw a rectangle without fill */
  void DrawRectNoFill(vtkContext2D *painter, float x0, float y0, float x1, float y1);

  /** Draw a circle or ellipse */
  void DrawCircle(vtkContext2D *painter,
                  float x, float y, float z,
                  bool fill, unsigned int n_segments,
                  float scale_x = 1.0, float scale_y = 1.0);

  /** Draw a string rectangle */
  void DrawStringRect(vtkContext2D *painter, const std::string &text,
                      double x, double y, double w, double h,
                      const AbstractRendererPlatformSupport::FontInfo &fi,
                      int halign, int valign,
                      const Vector3d &color, double alpha = 1.0);
protected:

  // Get the viewport pixel ratio (useful for deciding on line sizes)
  double GetVPPR();

  // Pointer to the model
  GenericSliceModel *m_Model;
};


#endif // GENERICSLICECONTEXTITEM_H
