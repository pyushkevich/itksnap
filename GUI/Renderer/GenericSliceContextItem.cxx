#include "GenericSliceContextItem.h"
#include "GenericSliceModel.h"
#include "SNAPAppearanceSettings.h"
#include <vtkContext2D.h>
#include <vtkPoints2D.h>
#include <vtkPen.h>
#include <vtkTextProperty.h>
#include <vtkImageData.h>
#include "AbstractRenderer.h"

void GenericSliceContextItem
::ApplyAppearanceSettingsToPen(
    vtkContext2D *painter, const OpenGLAppearanceElement *as)
{
  painter->GetPen()->SetColorF(as->GetColor().data_block());
  painter->GetPen()->SetWidth(as->GetLineThickness() * GetVPPR());
  painter->GetPen()->SetOpacityF(as->GetAlpha());
  painter->GetPen()->SetLineType(as->GetLineType());
}

void GenericSliceContextItem
::ApplyAppearanceSettingsToFont(
    vtkContext2D *painter, const OpenGLAppearanceElement *as)
{
  painter->GetTextProp()->SetFontSize(as->GetFontSize() * GetVPPR());
  painter->GetTextProp()->SetColor(as->GetColor().data_block());
  painter->GetTextProp()->SetOpacity(as->GetAlpha());
}

void GenericSliceContextItem::DrawRectNoFill(
    vtkContext2D *painter, float x0, float y0, float x1, float y1)
{
  // Use drawpoly method because DrawRect includes a fill
  float p[] = { x0, y0,
                x0, y1,
                x1, y1,
                x1, y0,
                x0, y0 };

  painter->DrawPoly(p, 5);
}

#include <vtkTransform2D.h>

void
GenericSliceContextItem
::DrawStringRect(vtkContext2D *painter, const std::string &text,
                 double x, double y, double w, double h,
                 const AbstractRendererPlatformSupport::FontInfo &fi,
                 int halign, int valign,
                 const Vector3d &color, double alpha)
{
  AbstractRendererPlatformSupport *sup = AbstractRenderer::GetPlatformSupport();

  // Get the screen coordinates of the rectangle
  auto *mtx = painter->GetTransform()->GetMatrix();
  double xscr0 = mtx->GetElement(0,0) * x + mtx->GetElement(0,1) * y + mtx->GetElement(0,2);
  double yscr0 = mtx->GetElement(1,0) * x + mtx->GetElement(1,1) * y + mtx->GetElement(1,2);
  double xscr1 = mtx->GetElement(0,0) * (x+w) + mtx->GetElement(0,1) * (y+h) + mtx->GetElement(0,2);
  double yscr1 = mtx->GetElement(1,0) * (x+w) + mtx->GetElement(1,1) * (y+h) + mtx->GetElement(1,2);

  double xscr = std::min(xscr0, xscr1), yscr = std::min(yscr0, yscr1);
  int wscr = int(std::fabs(0.5 + xscr1 - xscr0));
  int hscr = int(std::fabs(0.5 + yscr1 - yscr0));

  // Keep some string images cached.
  typedef std::tuple<
      std::string, int, int,
      int, int, bool,
      int, int, double, double, double, double> Key;

  static std::map<Key, vtkSmartPointer<vtkImageData> > cache;

  vtkSmartPointer<vtkImageData> img;
  Key key = std::make_tuple(text, wscr, hscr,
                            (int) fi.type, fi.pixel_size, fi.bold,
                            halign, valign, color[0], color[1], color[2], alpha);
  auto it = cache.find(key);
  if(it == cache.end())
    {
    // Check cache size. If above a threshold, we delete the whole cache and start over.
    // This is a little lazy, but we're talking about miniscule operations here
    if(cache.size() > 512)
      cache.clear();

    // Allocate a vtkImageData for this text. TODO: allow caching based on the
    // hash of the parameters and text
    img = vtkSmartPointer<vtkImageData>::New();
    img->SetDimensions(wscr, hscr, 1);
    img->AllocateScalars(VTK_UNSIGNED_CHAR, 4);
    sup->RenderTextIntoVTKImage(text.c_str(), img, fi, halign, valign, color, alpha);
    cache[key] = img;
    }
  else
    {
    img = it->second;
    }

  painter->PushMatrix();
  vtkNew<vtkTransform2D> tform;
  tform->Identity();
  painter->SetTransform(tform);
  painter->DrawImage(xscr, yscr, img);
  painter->PopMatrix();
}

double GenericSliceContextItem::GetVPPR()
{
  return m_Model->GetSizeReporter()->GetViewportPixelRatio();
}
