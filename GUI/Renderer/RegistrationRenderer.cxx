#include "RegistrationRenderer.h"
#include "InteractiveRegistrationModel.h"
#include "GenericSliceModel.h"
#include "RegistrationModel.h"
#include "SNAPAppearanceSettings.h"
#include "SNAPOpenGL.h"
#include "GlobalUIModel.h"

RegistrationRenderer::RegistrationRenderer()
{
  m_Model = NULL;
}

RegistrationRenderer::~RegistrationRenderer()
{

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

  // Get the line color, thickness and dash spacing for the rotation widget
  OpenGLAppearanceElement *eltWidgets =
    as->GetUIElement(SNAPAppearanceSettings::REGISTRATION_WIDGETS);

  // TODO: it would be nice to only draw the rotation widget when rendering the moving
  // image layer and none of the other layers. But for now we draw this in every tile

  // The rotation widget is a circular arc that is drawn around the center of rotation
  // The radius of the arc is chosen so that there is maximum overlap between the arc
  // and the screen area, minus a margin. For now though, we compute the radius in a
  // very heuristic way
  double radius = m_Model->GetRotationWidgetRadius();

  // Get the center of rotation
  Vector3ui rot_ctr_image = rmodel->GetRotationCenter();

  // Map the center of rotation into the slice coordinates
  Vector3f rot_ctr_slice = smodel->MapImageToSlice(to_float(rot_ctr_image));

  // Set line properties
  glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);
  glPushMatrix();

  // The matrix is configured in the slice coordinate system. However, to draw the
  // circle, we should be in a coordinate system where the origin is the center of
  // rotation,

  glTranslated(rot_ctr_slice[0], rot_ctr_slice[1], 0.0);
  glScaled(0.5 * radius / smodel->GetSliceSpacing()[0], 0.5 * radius / smodel->GetSliceSpacing()[1], 1.0);

  // Draw a white circle
  if(m_Model->IsHoveringOverRotationWidget())
    glColor3dv(eltWidgets->GetActiveColor().data_block());
  else
    glColor3dv(eltWidgets->GetNormalColor().data_block());
  eltWidgets->ApplyLineSettings();

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


  glPopMatrix();
  glPopAttrib();
}

