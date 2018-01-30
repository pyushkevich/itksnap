#include "SnakeParameterPreviewRenderer.h"
#include "SnakeParametersPreviewPipeline.h"
#include "OpenGLSliceTexture.h"
#include "SnakeParameterModel.h"

SnakeParameterPreviewRenderer::SnakeParameterPreviewRenderer()
{
  // Initialize the texture object
  m_Texture = new OpenGLSliceTexture<RGBAType>;
  m_Texture->SetGlComponents(4);
  m_Texture->SetGlFormat(GL_RGBA);
}

SnakeParameterPreviewRenderer::~SnakeParameterPreviewRenderer()
{
  delete m_Texture;
}

void SnakeParameterPreviewRenderer::initializeGL()
{
}

void SnakeParameterPreviewRenderer::resizeGL(int w, int h, int device_pixel_ratio)
{
  // The window will have coordinates (0,0) to (1,1)
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  irisOrtho2D(0.0,1.0,0.0,1.0);
  glViewport(0,0,w,h);

  // Establish the model view matrix
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  m_ViewportWidth = w;
}

void SnakeParameterPreviewRenderer::SetModel(SnakeParameterModel *model)
{
  m_Model = model;
  m_Pipeline = model->GetPreviewPipeline();
  m_Texture->SetImage(m_Pipeline->GetDisplayImage());

  Rebroadcast(m_Model, ModelUpdateEvent(), ModelUpdateEvent());
  Rebroadcast(m_Model->GetAnimateDemoModel(), ValueChangedEvent(), ModelUpdateEvent());

  Rebroadcast(m_Model, SnakeParameterModel::DemoLoopEvent(), ModelUpdateEvent());
}

void SnakeParameterPreviewRenderer::paintGL()
{
  // Update everything
  m_Pipeline->Update();

  // Clear the display
  glClearColor(0.0,0.0,0.0,1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Set up the line drawing attributes
  glPushAttrib(GL_LIGHTING_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT);

  // Set up the model matrix
  glPushMatrix();
  glScaled(1.0 / m_Pipeline->GetSpeedImage()->GetBufferedRegion().GetSize(0),
           1.0 / m_Pipeline->GetSpeedImage()->GetBufferedRegion().GetSize(1),
           1.0);

  // Draw the speed image
  m_Texture->Draw(Vector3d(1.0));

  // Set up the line drawing mode
  glEnable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glLineWidth(2.0);
  glColor3d(1.0, 0.0, 0.0);

  // Draw the evolving contour if it's available
  if(m_Model->GetAnimateDemo())
    {
    std::vector<Vector2d> &points = m_Pipeline->GetDemoLoopContour();
    glColor3d(1.0, 0.0, 0.0);
    glBegin(GL_LINES);
    std::vector<Vector2d>::iterator it = points.begin();
    for(; it != points.end(); ++it)
      {
      glVertex(*it);
      }
    glEnd();

    glLineWidth(1.0);
    glColor4d(1.0, 0.0, 0.0, 0.5);
    }


  // Draw the vectors
  //glLineWidth(1.0);

  // No more image scaling
  glPopMatrix();
  glPushMatrix();

  // Get the point collection
  const SnakeParametersPreviewPipeline::SampledPointList
      &list = m_Pipeline->GetSampledPoints();

  // Draw the spline
  glBegin(GL_LINE_LOOP);
  for(unsigned int j=0;j<list.size();j++)
    {
    glVertex2d(list[j].x[0],list[j].x[1]);
    }
  glEnd();

  // Draw the forces on the spline
  // Draw the vectors from the curve
  glColor3d(1.0,0.0,0.0);
  glBegin(GL_LINES);
  for(unsigned int i=0;i<list.size();i+=4)
    {
    // A reference so we can access the point in shorthand
    const SnakeParametersPreviewPipeline::SampledPoint &p = list[i];

    // Decide which force to draw, depending on the current state
    double force = 0;
    switch(m_ForceToDisplay)
      {
      case PROPAGATION_FORCE :
        force = p.PropagationForce;
        break;
      case CURVATURE_FORCE :
        force = p.CurvatureForce;
        break;
      case ADVECTION_FORCE :
        force = p.AdvectionForce;
        break;
      case TOTAL_FORCE :
        force = p.PropagationForce + p.CurvatureForce + p.AdvectionForce;
        break;
      }

    // Scale the force for effect
    force *= 10;

    // Draw the forces
    glVertex2d(p.x[0],p.x[1]);
    glVertex2d(p.x[0] + force * p.n[0] / m_ViewportWidth,
               p.x[1] + force * p.n[1] / m_ViewportWidth);
    }

  glEnd();

  /*
    const SnakeParametersPreviewPipeline::ImagePointList
      &plist = m_Pipeline->GetImagePoints();

    glBegin(GL_LINES);
    for(unsigned int j=0;j<plist.size();j++)
      {
      const SnakeParametersPreviewPipeline::PointInfo &point = plist[j];

      float length = 10.0f;
      switch(m_ForceToDisplay)
        {
        case CURVATURE_FORCE   : length *= point.CurvatureForce; break;
        case ADVECTION_FORCE   : length *= point.AdvectionForce; break;
        case PROPAGATION_FORCE : length *= point.PropagationForce; break;
        case TOTAL_FORCE       :
          length *= (point.CurvatureForce + point.AdvectionForce +
                     point.PropagationForce); break;
        }

      SnakeParametersPreviewPipeline::SampledPoint p = point.point;
      glVertex2d(p.x[0],p.x[1]);
      glVertex2d(p.x[0] + length * p.n[0],p.x[1] + length * p.n[1]);
      }
    glEnd();
  */

  // TODO: Draw the interactor

  // Pop the matrix
  glPopMatrix();

  // Restore the attribute state
  glPopAttrib();
}
