#include "SnakeModeRenderer.h"
#include "SnakeWizardModel.h"
#include "GenericSliceModel.h"
#include "GlobalUIModel.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "SNAPImageData.h"
#include "GlobalState.h"
#include "itkImage.h"
#include "GenericSliceContextItem.h"
#include <vtkObjectFactory.h>
#include <vtkContext2D.h>
#include <vtkPen.h>
#include <vtkBrush.h>
#include <vtkPoints2D.h>
#include <vtkTransform2D.h>
#include "VTKRenderGeometry.h"

class SnakeModeContextItem : public GenericSliceContextItem
{
public:
  vtkTypeMacro(SnakeModeContextItem, GenericSliceContextItem)
  static SnakeModeContextItem *New();

  irisGetSetMacro(SnakeWizardModel, SnakeWizardModel *)

  SnakeModeContextItem()
  {
    // Generate circle geometry
    m_Circle = VTKRenderGeometry::MakeUnitCircle(120);
    m_QuadStrip = VTKRenderGeometry::MakeUnitDisk(120);
  }

  virtual void DrawBubbles(vtkContext2D *painter)
  {
    IRISApplication *app = m_SnakeWizardModel->GetParent()->GetDriver();
    GlobalState *gs = app->GetGlobalState();

    // Get the list of bubbles
    IRISApplication::BubbleArray &bubbles = app->GetBubbleArray();

    // draw bubbles
    int numBubbles = bubbles.size();
    int activeBubble = gs->GetActiveBubble();

    if (numBubbles > 0)
      {
      // Get the active color label
      int currentcolor = gs->GetDrawingColorLabel();
      ColorLabel cl = app->GetColorLabelTable()->GetColorLabel(currentcolor);

      // Get the current alpha blending factor for displaying overlays
      unsigned char alpha = (unsigned char)(255 * gs->GetSegmentationAlpha());

      // Get the color of the active color label
      Vector3d clrFill = cl.GetRGBAsDoubleVector();
      Vector3d clrWhite(1.0, 1.0, 1.0);
      Vector3d clrLine = clrWhite - (clrWhite - clrFill) / 2.0;

      // Get the current crosshairs position
      Vector3d cursorImage = to_double(app->GetCursorPosition()) + Vector3d(0.5);

      // Get the image space dimension that corresponds to this window
      int iid = m_Model->GetSliceDirectionInImageSpace();

      // Get the other essentials from the parent
      Vector3d scaling = m_Model->GetSliceSpacing();

      // Draw each bubble
      for (int i = 0; i < numBubbles; i++)
        {
        // Get the center and radius of the i-th bubble
        Vector3d ctrImage = to_double(bubbles[i].center) + Vector3d(0.5);
        double radius = bubbles[i].radius;

        // Remap the center into slice coordinates
        Vector3d ctrSlice = m_Model->MapImageToSlice(ctrImage);

        // Compute the offset from the center along the slice z-direction
        // in physical coordinates
        double dcenter = scaling(2) * (cursorImage(iid) - ctrImage(iid));

        // Check if the bubble is intersected by the current slice plane
        if (dcenter >= radius || -dcenter >= radius) continue;

        // Compute the radius of the bubble in the cut plane
        double diskradius = sqrt(fabs(radius*radius - dcenter*dcenter));

        // Create a transform for this bubble
        vtkNew<vtkTransform2D> tform;
        tform->Translate(ctrSlice[0], ctrSlice[1]);
        tform->Scale(diskradius / scaling[0], diskradius / scaling[1]);
        painter->PushMatrix();
        painter->AppendTransform(tform);

        // Draw filled region geometry
        painter->GetBrush()->SetColorF(clrFill.data_block());
        painter->GetBrush()->SetOpacityF(alpha / 512.);
        painter->DrawQuadStrip(m_QuadStrip);

        // Draw outline
        if(i == activeBubble)
          {
          painter->GetPen()->SetColorF(clrLine.data_block());
          painter->GetPen()->SetWidth(1.5 * GetVPPR());
          painter->GetPen()->SetOpacityF(1.0);
          painter->GetPen()->SetLineType(vtkPen::SOLID_LINE);
          painter->DrawPoly(m_Circle);
          }

        // Release matrix
        painter->PopMatrix();
        }
      }
  }

  virtual bool Paint(vtkContext2D *painter) override
  {
    IRISApplication *app = m_SnakeWizardModel->GetParent()->GetDriver();

    if(app->IsSnakeModeActive())
      {
      // Bubbles are drawn only when on the bubbles page
      if(m_SnakeWizardModel->CheckState(SnakeWizardModel::UIF_BUBBLE_MODE))
        {
        // Draw the bubbles before segmentation starts
        this->DrawBubbles(painter);
        }
      }

    return true;
  }

protected:

  SnakeWizardModel *m_SnakeWizardModel;

  vtkSmartPointer<vtkPoints2D> m_Circle, m_QuadStrip;

};

vtkStandardNewMacro(SnakeModeContextItem);


SnakeModeRenderer::SnakeModeRenderer()
{
  m_Model = NULL;
}

void SnakeModeRenderer::AddContextItemsToTiledOverlay(
    vtkAbstractContextItem *parent, ImageWrapperBase *)
{
  if(m_Model)
    {
    vtkNew<SnakeModeContextItem> ci;
    ci->SetSnakeWizardModel(m_Model);
    ci->SetModel(this->GetParentRenderer()->GetModel());
    parent->AddItem(ci);
    }
}
