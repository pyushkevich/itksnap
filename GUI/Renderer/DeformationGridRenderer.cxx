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

bool
DeformationGridContextItem
::Paint(vtkContext2D *painter)
{
  //std::cout << "[DeformationGridContextItem] Paint" << std::endl;

  auto vplayout = m_Model->GetViewportLayout();
  //hack
  auto vp = vplayout.vpList[0];
  DeformationGridVertices verts;
  m_DeformationGridModel->GetVertices(vp, verts);

  auto as = m_Model->GetParentUI()->GetAppearanceSettings();
  auto elt = as->GetUIElement(SNAPAppearanceSettings::GRID_LINES);
  this->ApplyAppearanceSettingsToPen(painter, elt);


//  std::cout << "[DGPaint] d0: (" << verts.d0_nline << ',' << verts.d0_nvert
//            << ") d1:(" << verts.d1_nline << ',' << verts.d1_nvert
//            << ") total0: " << verts.vlist[0].size()
//            << "; total1: " << verts.vlist[1].size()
//            << std::endl;

  //std::cout << "-- vertices count: " << vertices.size() << std::endl;

  // testing with horizontal lines only

  vtkNew<vtkPoints2D> pts;
  pts->Allocate(verts.d0_nvert * verts.d0_nline);

  bool reverse = false;

  for (size_t l = 0; l < verts.d0_nline; ++l)
    {
    if (!reverse)
      for (size_t v = 0; v < verts.d0_nvert; ++v)
        {
        pts->InsertNextPoint(
              verts.vvec[(l * verts.d0_nvert + v) * 2],
              verts.vvec[(l * verts.d0_nvert + v) * 2 + 1]
            );
        }
    else
      for (long long v = verts.d0_nvert - 1; v >= 0; --v)
        {
        pts->InsertNextPoint(
              verts.vvec[(l * verts.d0_nvert + v) * 2],
              verts.vvec[(l * verts.d0_nvert + v) * 2 + 1]
            );
        }
    reverse = !reverse;
    }
  painter->DrawPoly(pts);


  size_t jump = verts.d0_nline * verts.d0_nvert * 2;

  vtkNew<vtkPoints2D> pts1;
  pts->Allocate(verts.d1_nvert * verts.d1_nline);

  reverse = false;

  for (size_t l = 0; l < verts.d1_nline; ++l)
    {
    if (!reverse)
      for (size_t v = 0; v < verts.d1_nvert; ++v)
        {
        pts1->InsertNextPoint(
              verts.vvec[jump + (l * verts.d1_nvert + v) * 2],
              verts.vvec[jump + (l * verts.d1_nvert + v) * 2 + 1]
            );
        }
    else
      for (long long v = verts.d1_nvert - 1; v >= 0; --v)
        {
        pts1->InsertNextPoint(
              verts.vvec[jump + (l * verts.d1_nvert + v) * 2],
              verts.vvec[jump + (l * verts.d1_nvert + v) * 2 + 1]
            );
        }
    reverse = !reverse;
    }
  painter->DrawPoly(pts1);

//  vtkNew<vtkPoints2D> polyline;
//  polyline->Allocate(vertices.size());
//  for(auto &it : vertices)
//    polyline->InsertNextPoint(it.x, it.y);




  //painter->DrawPoly(polyline);

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
      vtkAbstractContextItem *parent, ImageWrapperBase *)
{
  if(m_Model)
    {
    vtkNew<DeformationGridContextItem> ci;
    ci->SetModel(m_Model->GetParent());
    ci->SetDeformationGridModel(m_Model);
    parent->AddItem(ci);
    }
}
