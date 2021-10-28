#include "PaintbrushRenderer.h"
#include "PaintbrushModel.h"
#include "IRISApplication.h"
#include "GlobalState.h"
#include "SNAPAppearanceSettings.h"
#include "GlobalUIModel.h"
#include "GenericSliceContextItem.h"
#include <vtkObjectFactory.h>
#include <vtkContext2D.h>
#include <vtkPoints2D.h>
#include <vtkPen.h>


class PaintbrushContextItem : public GenericSliceContextItem
{
public:
  vtkTypeMacro(PaintbrushContextItem, GenericSliceContextItem)
  static PaintbrushContextItem *New();

  irisSetMacro(PaintbrushModel, PaintbrushModel *);
  irisGetMacro(PaintbrushModel, PaintbrushModel *);


  void BuildBrush()
  {
    // Get the current properties
    GlobalState *gs = m_Model->GetDriver()->GetGlobalState();
    PaintbrushSettings ps = gs->GetPaintbrushSettings();

    // This is a simple 2D marching algorithm. At any given state of the
    // marching, there is a 'tail' and a 'head' of an arrow. To the right
    // of the arrow is a voxel that's inside the brush and to the left a
    // voxel that's outside. Depending on the two voxels that are
    // ahead of the arrow to the left and right (in in, in out, out out)
    // at the next step the arrow turns right, continues straight or turns
    // left. This goes on until convergence

    // Initialize the marching. This requires constructing the first arrow
    // and marching it to the left until it is between out and in voxels.
    // If the brush has even diameter, the arrow is from (0,0) to (1,0). If
    // the brush has odd diameter (center at voxel center) then the arrow
    // is from (-0.5, -0.5) to (-0.5, 0.5)
    Vector2d xTail, xHead;
    if(fmod(ps.radius,1.0) == 0)
      { xTail = Vector2d(0.0, 0.0); xHead = Vector2d(0.0, 1.0); }
    else
      { xTail = Vector2d(-0.5, -0.5); xHead = Vector2d(-0.5, 0.5); }

    // Shift the arrow to the left until it is in position

    while(m_PaintbrushModel->TestInside(Vector2d(xTail(0) - 0.5, xTail(1) + 0.5), ps))
      { xTail(0) -= 1.0; xHead(0) -= 1.0; }

    // Record the starting point, which is the current tail. Once the head
    // returns to the starting point, the loop is done
    Vector2d xStart = xTail;

    // Do the loop
    m_Walk.clear();
    size_t n = 0;
    while((xHead - xStart).squared_magnitude() > 0.01 && (++n) < 10000)
      {
      // Add the current head to the loop
      m_Walk.push_back(xHead);

      // Check the voxels ahead to the right and left
      Vector2d xStep = xHead - xTail;
      Vector2d xLeft(-xStep(1), xStep(0));
      Vector2d xRight(xStep(1), -xStep(0));
      bool il = m_PaintbrushModel->TestInside(xHead + 0.5 * (xStep + xLeft),ps);
      bool ir = m_PaintbrushModel->TestInside(xHead + 0.5 * (xStep + xRight),ps);

      // Update the tail
      xTail = xHead;

      // Decide which way to go
      if(il && ir)
        xHead += xLeft;
      else if(!il && ir)
        xHead += xStep;
      else if(!il && !ir)
        xHead += xRight;
      else
        assert(0);
      }

    // Add the last vertex
    m_Walk.push_back(xStart);
  }

  virtual bool Paint(vtkContext2D *painter) override
  {
    // Check if the mouse is inside
    if(!m_PaintbrushModel->IsMouseInside())
      return false;

    // Paint all the edges in the paintbrush definition
    auto *as = m_Model->GetParentUI()->GetAppearanceSettings();
    const auto *elt = as->GetUIElement(SNAPAppearanceSettings::PAINTBRUSH_OUTLINE);

    // Build the mask edges
    BuildBrush();

    // Apply the line properties
    this->ApplyAppearanceSettingsToPen(painter, elt);

    // Get the brush position
    Vector3d xPos = m_PaintbrushModel->GetCenterOfPaintbrushInSliceSpace();

    if(m_Walk.size() > 1)
      {
      vtkNew<vtkPoints2D> walk;
      walk->Allocate(m_Walk.size() + 1);

      for(auto it : m_Walk)
        walk->InsertNextPoint(it[0] + xPos[0], it[1] + xPos[1]);

      walk->InsertNextPoint(walk->GetPoint(0));

      painter->DrawLines(walk);
      }

    return true;
  }

protected:

  PaintbrushModel *m_PaintbrushModel;

  // Representation of the brush
  std::list<Vector2d> m_Walk;
};

vtkStandardNewMacro(PaintbrushContextItem);



void PaintbrushRenderer::AddContextItemsToTiledOverlay(
    vtkAbstractContextItem *parent, ImageWrapperBase *)
{
  if(m_Model)
    {
    vtkNew<PaintbrushContextItem> ci;
    ci->SetModel(m_Model->GetParent());
    ci->SetPaintbrushModel(m_Model);
    parent->AddItem(ci);
    }
}
