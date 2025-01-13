#include "RegistrationRenderer.h"
#include "InteractiveRegistrationModel.h"
#include "GenericSliceModel.h"
#include "RegistrationModel.h"
#include "SNAPAppearanceSettings.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "ImageWrapperBase.h"
#include <vtkObjectFactory.h>

void RegistrationRenderer::DrawRotationWidget(AbstractRenderContext *context, double beta)
{
  GenericSliceModel *slice_model = m_Model->GetParent();

  // The rotation widget is a circular arc that is drawn around the center of rotation
  // The radius of the arc is chosen so that there is maximum overlap between the arc
  // and the screen area, minus a margin. For now though, we compute the radius in a
  // very heuristic way
  double radius = m_Model->GetRotationWidgetRadius();

  // Get the center of rotation
  Vector3ui rot_ctr_image = m_Model->GetRegistrationModel()->GetRotationCenter();

  // Map the center of rotation into the slice coordinates
  Vector3d rot_ctr_slice = slice_model->MapImageToSlice(to_double(rot_ctr_image));

  // Get the scale parameters
  double sx = 0.5 * radius / slice_model->GetSliceSpacing()[0];
  double sy = 0.5 * radius / slice_model->GetSliceSpacing()[1];

  // Create a rotation widget first time
  if(!m_RotatorPath)
  {
    m_RotatorPath = context->CreatePath();
    unsigned int n = 360, nr = 72;

    std::vector<Vector2d> circle;
    circle.reserve(n);

    for(unsigned int i = 0; i < n; i++)
    {
      double alpha = i * 2 * vnl_math::pi / n, x = cos(alpha), y = sin(alpha);
      circle.push_back(Vector2d(x, y));
    }
    context->AddPolygonSegmentToPath(m_RotatorPath, circle, true);

    std::vector<Vector2d> spoke(2);
    for(unsigned int i = 0; i <= nr; i++)
    {
      double alpha = i * 2 * vnl_math::pi / nr, x = cos(alpha), y = sin(alpha);
      spoke[0] = Vector2d(0.95 * x, 0.95 * y);
      spoke[1] = Vector2d(1.05 * x, 1.05 * y);
      context->AddPolygonSegmentToPath(m_RotatorPath, spoke, false);
    }
  }

  // Set transform for the wheel
  context->PushMatrix();
  context->Translate(rot_ctr_slice[0], rot_ctr_slice[1]);
  context->Scale(sx, sy);
  context->Rotate(- beta * 180 / vnl_math::pi);
  context->DrawPath(m_RotatorPath);
  context->PopMatrix();
}

void
RegistrationRenderer::DrawGrid(AbstractRenderContext *context)
{
  GenericSliceModel *slice_model = m_Model->GetParent();

  // Get the dimensions of the viewport that should be gridded
  Vector2ui vp_pos, vp_size;
  slice_model->GetNonThumbnailViewport(vp_pos, vp_size);

  // Generate the grid path
  int spacing = 16 * context->GetDevicePixelRatio();

  if(m_Grid.size() == 0 || m_GridViewportPos != vp_pos || m_GridViewportSize != vp_size)
  {
    m_Grid.clear();
    m_Grid.reserve(vp_size[0] / spacing + vp_size[1] / spacing);
    m_GridViewportPos = vp_pos;
    m_GridViewportSize = vp_size;

    std::vector<Vector2d> line(2);
    for (unsigned int i = 0; i <= vp_size[0]; i += spacing)
    {
      m_Grid.push_back(Vector2d(vp_pos[0] + i, vp_pos[1]));
      m_Grid.push_back(Vector2d(vp_pos[0] + i, vp_pos[1] + vp_size[1]));
    }

    for (unsigned int i = 0; i <= vp_size[1]; i += spacing)
    {
      m_Grid.push_back(Vector2d(vp_pos[0], vp_pos[1] + i));
      m_Grid.push_back(Vector2d(vp_pos[0] + vp_size[0], vp_pos[1] + i));
    }
  }

  // Get the registration grid lines appearance
  auto *as = slice_model->GetParentUI()->GetAppearanceSettings();
  auto *eltGrid = as->GetUIElement(SNAPAppearanceSettings::REGISTRATION_GRID);
  context->SetPenAppearance(*eltGrid);

  // Gridlines should be drawn in window space, not image space
  context->PushMatrix();
  context->LoadIdentity();
  context->DrawLines(m_Grid);
  context->PopMatrix();
}

void
RegistrationRenderer::RenderOverTiledLayer(AbstractRenderContext *context,
                                              ImageWrapperBase         *base_layer,
                                              const SubViewport        &vp)
{
  // Find out what layer is being used for registration
  RegistrationModel *rmodel = m_Model->GetRegistrationModel();
  SNAPAppearanceSettings *as = m_Model->GetParent()->GetParentUI()->GetAppearanceSettings();
  ImageWrapperBase *moving = rmodel->GetMovingLayerWrapper();
  if(vp.isThumbnail || !moving)
    return;

  // Draw the grid
  this->DrawGrid(context);

  // Should we draw the widget? Yes, if we are in tiled mode and are viewing the moving layer,
  // and yes if we are in non-tiled mode.
  unsigned long drawing_id = base_layer->GetUniqueId();
  if(m_Model->GetDoProcessInteractionOverLayer(drawing_id))
  {
    // Get the line color, thickness and dash spacing for the rotation widget
    OpenGLAppearanceElement *eltWidgets =
      as->GetUIElement(SNAPAppearanceSettings::REGISTRATION_WIDGETS);

    // Get the line color, thickness and dash spacing for the rotation widget
    OpenGLAppearanceElement *eltWidgetsActive =
      as->GetUIElement(SNAPAppearanceSettings::REGISTRATION_WIDGETS_ACTIVE);

    // Draw a white circle
    if(m_Model->IsHoveringOverRotationWidget())
    {
      if(m_Model->GetLastTheta() != 0.0)
      {
        context->SetPenAppearance(*eltWidgets);
        this->DrawRotationWidget(context, 0);

        context->SetPenAppearance(*eltWidgetsActive);
        this->DrawRotationWidget(context, m_Model->GetLastTheta());
      }
      else
      {
        context->SetPenAppearance(*eltWidgetsActive);
        this->DrawRotationWidget(context, 0);
      }
    }
    else
    {
      context->SetPenAppearance(*eltWidgets);
      this->DrawRotationWidget(context, 0);
    }
  }
}
