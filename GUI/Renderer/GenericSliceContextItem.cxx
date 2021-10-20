#include "GenericSliceContextItem.h"
#include "GenericSliceModel.h"
#include "SNAPAppearanceSettings.h"
#include <vtkContext2D.h>
#include <vtkPen.h>

void GenericSliceContextItem
::ApplyAppearanceSettingsToPen(
    vtkContext2D *painter, const OpenGLAppearanceElement *as)
{
  painter->GetPen()->SetColorF(as->GetColor().data_block());
  painter->GetPen()->SetWidth(as->GetLineThickness() * GetVPPR());
  painter->GetPen()->SetOpacityF(as->GetAlpha());
  if(as->GetDashSpacing() >= 4)
    painter->GetPen()->SetLineType(vtkPen::DASH_DOT_DOT_LINE);
  else if(as->GetDashSpacing() >= 3)
    painter->GetPen()->SetLineType(vtkPen::DASH_DOT_LINE);
  else if(as->GetDashSpacing() >= 2)
    painter->GetPen()->SetLineType(vtkPen::DOT_LINE);
  else if(as->GetDashSpacing() >= 1)
    painter->GetPen()->SetLineType(vtkPen::DASH_LINE);
  else
    painter->GetPen()->SetLineType(vtkPen::SOLID_LINE);
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

double GenericSliceContextItem::GetVPPR()
{
  return m_Model->GetSizeReporter()->GetViewportPixelRatio();
}
