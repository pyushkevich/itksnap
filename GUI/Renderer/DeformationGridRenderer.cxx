#include "DeformationGridRenderer.h"
#include "GlobalUIModel.h"
#include "SNAPAppearanceSettings.h"
#include <vector>
#include <vtkContext2D.h>
#include <vtkPoints2D.h>
#include <vtkPen.h>

//=================================================
// DeformationGridContextItem Definition
//=================================================
vtkStandardNewMacro(DeformationGridContextItem)

DeformationGridContextItem
::DeformationGridContextItem()
{

}

void DeformationGridContextItem
::AddLine(vtkPoints2D *pv, std::vector<double> &verts,
          size_t skip, size_t l, size_t nv, bool reverse)
{
  if (!reverse)
    for (size_t i = 0; i < nv; ++i)
      {
      pv->InsertNextPoint(
            verts[skip + (l * nv + i) * 2],
            verts[skip + (l * nv + i) * 2 + 1]
          );
      }
  else
    for (long long v = nv - 1; v >= 0; --v)
      {
      pv->InsertNextPoint(
            verts[skip + (l * nv + v) * 2],
            verts[skip + (l * nv + v) * 2 + 1]
          );
      }
}

bool
DeformationGridContextItem
::Paint(vtkContext2D *painter)
{
  // AddContextItemsToTiledOverlay of the renderer should take care of this
  assert(m_ImageLayer);

  auto mcdmp = dynamic_cast<AbstractMultiChannelDisplayMappingPolicy*>(m_ImageLayer->GetDisplayMapping());
  if (!mcdmp || !mcdmp->GetDisplayMode().RenderAsGrid)
    return true;


  DeformationGridVertices verts;
  m_DeformationGridModel->GetVertices(m_ImageLayer, verts);

  auto as = m_Model->GetParentUI()->GetAppearanceSettings();
  auto elt = as->GetUIElement(SNAPAppearanceSettings::GRID_LINES);
  this->ApplyAppearanceSettingsToPen(painter, elt);

  // Draw horizontal (d = 0) then vertical (d = 1)
  size_t skip = 0;
  for (int d = 0; d < 2; ++d)
    {
    vtkNew<vtkPoints2D> pts;
    pts->Allocate(verts.nvert[d] * verts.nline[d]);
    bool reverse = false;

    for (size_t l = 0; l < verts.nline[d]; ++l)
      {
      AddLine(pts, verts.vvec, skip, l, verts.nvert[d], reverse);
      reverse = !reverse;
      }

    painter->DrawPoly(pts);
    skip += verts.nvert[d] * verts.nline[d] * 2; // skip verts for horizontal lines
    }

  return true;
}


//=================================================
// DeformationGridRenderer Definition
//=================================================

DeformationGridRenderer
::DeformationGridRenderer()
{

}

void
DeformationGridRenderer
::AddContextItemsToTiledOverlay(
      vtkAbstractContextItem *parent, ImageWrapperBase *image)
{
  if(m_Model)
    {
    vtkNew<DeformationGridContextItem> ci;
    ci->SetModel(m_Model->GetParent());
    ci->SetDeformationGridModel(m_Model);
    ci->SetImageLayer(image);
    parent->AddItem(ci);
    }
}
