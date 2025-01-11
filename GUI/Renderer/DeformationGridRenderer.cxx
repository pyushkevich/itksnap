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

void
DeformationGridNewRenderer::RenderOverTiledLayer(AbstractNewRenderContext *context,
                                                 ImageWrapperBase         *base_layer,
                                                 const SubViewport        &vp)
{
  // Get the key to access the stored path from the layer
  static const char *key_path[] = { "LayerGridlines_0", "LayerGridlines_1", "LayerGridlines_2" };
  auto layer_key = key_path[m_Model->GetParent()->GetId()];

  // AddContextItemsToTiledOverlay of the renderer should take care of this
  itkAssertOrThrowMacro(base_layer,
                        "Layer must exist in DeformationGridNewRenderer::RenderOverTiledLayer");

  auto mcdmp = dynamic_cast<AbstractMultiChannelDisplayMappingPolicy*>(base_layer->GetDisplayMapping());
  if (!mcdmp || !mcdmp->GetDisplayMode().RenderAsGrid)
    return;

  // Check the layer for a stored path
  SmartPtr<Path2D> stored_path = dynamic_cast<Path2D *>(base_layer->GetUserData(layer_key));

  // Check when the display slice was last updated (this is safe because this code is called
  // after rendering the display slice
  auto ds =
    base_layer->GetDisplaySlice(DisplaySliceIndex(m_Model->GetParent()->GetId(), DISPLAY_SLICE_MAIN));

  if (!stored_path || stored_path->GetMTime() < ds->GetMTime())
  {
    DeformationGridVertices verts;
    m_Model->GetVertices(base_layer, verts);

    // Create a new path.
    // TODO: cache this path between paints
    stored_path = context->CreatePath();

    // Draw horizontal (d = 0) then vertical (d = 1)
    size_t skip = 0;
    for (int d = 0; d < 2; ++d)
    {
      vtkNew<vtkPoints2D> pts;
      pts->Allocate(verts.nvert[d] * verts.nline[d]);
      bool reverse = false;

      for (size_t l = 0; l < verts.nline[d]; ++l)
      {
        AddLine(context, stored_path, verts.vvec, skip, l, verts.nvert[d], reverse);
        reverse = !reverse;
      }
      skip += verts.nvert[d] * verts.nline[d] * 2; // skip verts for horizontal lines
    }

    base_layer->SetUserData(layer_key, stored_path);
  }

  // Set the appearance
  auto as = m_Model->GetParent()->GetParentUI()->GetAppearanceSettings();
  auto elt = as->GetUIElement(SNAPAppearanceSettings::GRID_LINES);
  context->SetPenAppearance(*elt);

  // Draw the paths
  context->DrawPath(stored_path);
}

DeformationGridNewRenderer::DeformationGridNewRenderer()
{
}

void
DeformationGridNewRenderer::AddLine(AbstractNewRenderContext *context,
                                    Path2D                   *path,
                                    std::vector<double>      &verts,
                                    size_t                    skip,
                                    size_t                    l,
                                    size_t                    nv,
                                    bool                      reverse)
{
  std::vector<Vector2d> segment;
  segment.reserve(nv);
  if (!reverse)
    for (int i = 0; i < nv; ++i)
    {
      segment.push_back(Vector2d(verts[skip + (l * nv + i) * 2], verts[skip + (l * nv + i) * 2 + 1]));
    }
  else
    for (int v = nv - 1; v >= 0; --v)
    {
      segment.push_back(Vector2d(verts[skip + (l * nv + v) * 2], verts[skip + (l * nv + v) * 2 + 1]));
    }
  context->AddPolygonSegmentToPath(path, segment, false);
}
