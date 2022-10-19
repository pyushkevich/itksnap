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
  std::list<DeformationGridVertex> vertices;
  m_DeformationGridModel->GetVertices(vp, vertices);

  //std::cout << "-- vertices count: " << vertices.size() << std::endl;

  vtkNew<vtkPoints2D> polyline;
  polyline->Allocate(vertices.size());
  for(auto it : vertices)
    polyline->InsertNextPoint(it.x, it.y);


  auto as = m_Model->GetParentUI()->GetAppearanceSettings();
  auto elt = as->GetUIElement(SNAPAppearanceSettings::GRID_LINES);
  this->ApplyAppearanceSettingsToPen(painter, elt);

  painter->DrawPoly(polyline);

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
