#include "RegistrationRenderer.h"
#include "InteractiveRegistrationModel.h"
#include "GenericSliceModel.h"
#include "RegistrationModel.h"
#include "SNAPAppearanceSettings.h"
#include "SNAPOpenGL.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "ImageWrapperBase.h"
#include "OpenGLSliceTexture.h"

RegistrationRenderer::RegistrationRenderer()
{
  m_Model = NULL;
}

RegistrationRenderer::~RegistrationRenderer()
{

}

void RegistrationRenderer::DrawRotationWidget(const OpenGLAppearanceElement *ae)
{
  glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);
  ae->ApplyLineSettings();
  ae->ApplyColor();

  // Draw the main line
  glBegin(GL_LINE_LOOP);
  for(int i = 0; i < 360; i++)
    {
    double alpha = i * vnl_math::pi / 180;
    glVertex2d(cos(alpha), sin(alpha));
    }
  glEnd();

  // Draw the hash marks at 5 degree intervals
  glBegin(GL_LINES);
  for(int i = 0; i < 360; i+=5)
    {
    double alpha = i * vnl_math::pi / 180;
    double x = cos(alpha), y = sin(alpha);
    glVertex2d(0.95 * x, 0.95 * y);
    glVertex2d(1.05 * x, 1.05 * y);
    }
  glEnd();

  glPopAttrib();
}

void RegistrationRenderer::paintGL()
{
  assert(m_Model);

  // Not in thumbnail mode
  if(m_ParentRenderer->IsDrawingZoomThumbnail() || m_ParentRenderer->IsDrawingLayerThumbnail())
    return;

  // Find out what layer is being used for registration
  RegistrationModel *rmodel = m_Model->GetRegistrationModel();
  GenericSliceModel *smodel = m_Model->GetParent();
  ImageWrapperBase *moving = rmodel->GetMovingLayerWrapper();
  if(!moving)
    return;

  SNAPAppearanceSettings *as =
      smodel->GetParentUI()->GetAppearanceSettings();

  // Get the registration grid lines
  OpenGLAppearanceElement *eltGrid =
    as->GetUIElement(SNAPAppearanceSettings::REGISTRATION_GRID);

  // How to draw the registration grid? Should it just be every 5 voxels?
  glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);
  glPushMatrix();
  glLoadIdentity();

  // Apply the grid settings
  eltGrid->ApplyLineSettings();
  eltGrid->ApplyColor();

  // Draw gridlines at regular intervals (this is dumb)
  Vector2ui canvas = smodel->GetCanvasSize();

  int spacing = 16 * smodel->GetSizeReporter()->GetViewportPixelRatio();

  glBegin(GL_LINES);
  for(int i = 0; i <= canvas[0]; i+=spacing)
    {
    glVertex2i(i, 0);
    glVertex2i(i, canvas[1]);
    }

  for(int i = 0; i <= canvas[1]; i+=spacing)
    {
    glVertex2i(0, i);
    glVertex2i(canvas[0], i);
    }

  glEnd();
  glPopMatrix();
  glPopAttrib();

  // Should we draw the widget? Yes, if we are in tiled mode and are viewing the moving layer,
  // and yes if we are in non-tiled mode.
  unsigned long drawing_id = this->m_ParentRenderer->GetDrawingViewport()->layer_id;
  if(m_Model->GetDoProcessInteractionOverLayer(drawing_id))
    {
    // Get the line color, thickness and dash spacing for the rotation widget
    OpenGLAppearanceElement *eltWidgets =
      as->GetUIElement(SNAPAppearanceSettings::REGISTRATION_WIDGETS);

    // Get the line color, thickness and dash spacing for the rotation widget
    OpenGLAppearanceElement *eltWidgetsActive =
      as->GetUIElement(SNAPAppearanceSettings::REGISTRATION_WIDGETS_ACTIVE);

    // The rotation widget is a circular arc that is drawn around the center of rotation
    // The radius of the arc is chosen so that there is maximum overlap between the arc
    // and the screen area, minus a margin. For now though, we compute the radius in a
    // very heuristic way
    double radius = m_Model->GetRotationWidgetRadius();

    // Get the center of rotation
    Vector3ui rot_ctr_image = rmodel->GetRotationCenter();

    // Map the center of rotation into the slice coordinates
    Vector3d rot_ctr_slice = smodel->MapImageToSlice(to_double(rot_ctr_image));

    // Set line properties
    glPushMatrix();

    // The matrix is configured in the slice coordinate system. However, to draw the
    // circle, we should be in a coordinate system where the origin is the center of
    // rotation,

    glTranslated(rot_ctr_slice[0], rot_ctr_slice[1], 0.0);
    glScaled(0.5 * radius / smodel->GetSliceSpacing()[0], 0.5 * radius / smodel->GetSliceSpacing()[1], 1.0);

    // Draw a white circle

    if(m_Model->IsHoveringOverRotationWidget())
      {
      if(m_Model->GetLastTheta() != 0.0)
        {
        this->DrawRotationWidget(eltWidgets);

        glRotated(m_Model->GetLastTheta() * 180 / vnl_math::pi, 0.0, 0.0, 1.0);
        this->DrawRotationWidget(eltWidgetsActive);
        }
      else
        {
        this->DrawRotationWidget(eltWidgetsActive);
        }
      }
    else
      {
      this->DrawRotationWidget(eltWidgets);
      }

    glPopMatrix();
    }
}

