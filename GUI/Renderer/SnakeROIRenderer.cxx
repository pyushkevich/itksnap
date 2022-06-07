#include "SnakeROIRenderer.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "GlobalState.h"
#include "SnakeROIModel.h"
#include "SNAPAppearanceSettings.h"
#include "GlobalUIModel.h"
#include "GenericSliceContextItem.h"
#include <vtkObjectFactory.h>
#include <vtkContext2D.h>
#include <vtkPen.h>

class SnakeROIContextItem : public GenericSliceContextItem
{
public:
  vtkTypeMacro(SnakeROIContextItem, GenericSliceContextItem)
  static SnakeROIContextItem *New();

  irisSetMacro(SnakeROIModel, SnakeROIModel *);
  irisGetMacro(SnakeROIModel, SnakeROIModel *);

  virtual bool Paint(vtkContext2D *painter) override
  {
    // Get some global information
    IRISApplication *app = m_Model->GetDriver();
    GlobalState *gs = app->GetGlobalState();
    SNAPAppearanceSettings *as = m_Model->GetParentUI()->GetAppearanceSettings();

    // The region of interest should be in effect
    assert(gs->isSegmentationROIValid());

    // Compute the corners in slice coordinates
    Vector3d corner[2];
    m_SnakeROIModel->GetSystemROICorners(corner);

    // Check that the current slice is actually within the bounding box
    // int slice = m_Parent->m_SliceIndex;
    int dim = m_Model->GetSliceDirectionInImageSpace();
    int slice = m_Model->GetSliceIndex();
    int bbMin = gs->GetSegmentationROI().GetIndex(dim);
    int bbMax = bbMin + gs->GetSegmentationROI().GetSize(dim);

    // And if so, return without painting anything
    if(bbMin > slice || bbMax <= slice)
      return false;

    // Get the line color, thickness and dash spacing
    const auto *elt = as->GetUIElement(SNAPAppearanceSettings::ROI_BOX);

    // Get the line color, thickness and dash spacing
    const auto *eltActive = as->GetUIElement(SNAPAppearanceSettings::ROI_BOX_ACTIVE);

    // Draw each of the edges
    for(unsigned int dir=0;dir<2;dir++)
      {
      for(unsigned int i=0;i<2;i++)
        {
        // Select color according to edge state
        if(m_SnakeROIModel->GetHighlight().Highlighted[dir][i])
          this->ApplyAppearanceSettingsToPen(painter, eltActive);
        else
          this->ApplyAppearanceSettingsToPen(painter, elt);

        // Compute the vertices of the edge
        Vector2d x0,x1;
        m_SnakeROIModel->GetEdgeVertices(dir,i,x0,x1,corner);

        // Start drawing the lines
        painter->DrawLine(x0[0], x0[1], x1[0], x1[1]);
        }
      }
    return true;
  }

protected:

  SnakeROIModel *m_SnakeROIModel;

};

vtkStandardNewMacro(SnakeROIContextItem);


SnakeROIRenderer::SnakeROIRenderer()
{
}

void SnakeROIRenderer::AddContextItemsToTiledOverlay(
    vtkAbstractContextItem *parent, ImageWrapperBase *)
{
  if(m_Model)
    {
    vtkNew<SnakeROIContextItem> ci;
    ci->SetModel(m_Model->GetParent());
    ci->SetSnakeROIModel(m_Model);
    parent->AddItem(ci);
    }
}
