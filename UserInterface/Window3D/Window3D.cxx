/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: Window3D.cxx,v $
  Language:  C++
  Date:      $Date: 2009/10/26 16:40:19 $
  Version:   $Revision: 1.13 $
  Copyright (c) 2007 Paul A. Yushkevich
  
  This file is part of ITK-SNAP 

  ITK-SNAP is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  -----

  Copyright (c) 2003 Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information. 

=========================================================================*/
#include "Window3D.h"
#include "IRISImageData.h"
#include "SNAPImageData.h"
#include "UserInterfaceBase.h"
#include "GlobalState.h"
#include <iostream>
#include "IRISApplication.h"
#include "ImageRayIntersectionFinder.h"
#include "SNAPAppearanceSettings.h"
#include "FLTKCanvas.h"
#include "GenericSliceWindow.h"

#include <FL/glut.H>
#include <FL/fl_ask.H>

#include <vxl_version.h>
#if VXL_VERSION_DATE_FULL > 20040406
# include <vnl/vnl_cross.h>
# define itk_cross_3d vnl_cross_3d
#else
# define itk_cross_3d cross_3d
#endif

#include <vnl/vnl_det.h>

/** These classes are used internally for m_Ray intersection testing */
class LabelImageHitTester 
{
public:
  LabelImageHitTester(const ColorLabelTable *table = NULL)
  {
    m_LabelTable = table;
  }

  int operator()(LabelType label) const
  {
    assert(m_LabelTable);
    const ColorLabel &cl = m_LabelTable->GetColorLabel(label);
    return (cl.IsValid() && cl.IsVisible() && cl.IsVisibleIn3D()) ? 1 : 0;
  }

private:
  const ColorLabelTable *m_LabelTable;
};

class SnakeImageHitTester 
{
public:
  int operator()(float levelSetValue) const
  {
    return levelSetValue <= 0 ? 1 : 0;
  }
};

/**
 * \class Trackball3DInteractionMode
 * \brief 3D interaction mode that takes care of 3D rotation and zoom
 *
 * \see Window3D
 */
class Trackball3DInteractionMode : public Window3D::EventHandler {
public:
  Trackball3DInteractionMode(Window3D *parent) : 
    Window3D::EventHandler(parent) {}

  int OnMousePress(const FLTKEvent &event);
  int OnMouseRelease(const FLTKEvent &event, const FLTKEvent &pressEvent);    
  int OnMouseDrag(const FLTKEvent &event, const FLTKEvent &pressEvent);
};

int 
Trackball3DInteractionMode
::OnMousePress(const FLTKEvent &event)
{
  int x = event.XCanvas[0];
  int y = event.XCanvas[1];

  switch (event.SoftButton)
    {
    case FL_LEFT_MOUSE:   m_Parent->OnRotateStartAction(x,y);break;
    case FL_MIDDLE_MOUSE: m_Parent->OnPanStartAction(x,y);break;
    case FL_RIGHT_MOUSE:  m_Parent->OnZoomStartAction(x,y);break;
    default: return 0;
    }

  return 1;
}

int 
Trackball3DInteractionMode
::OnMouseRelease(const FLTKEvent &irisNotUsed(event),
                 const FLTKEvent &irisNotUsed(startEvent))
{
  m_Parent->OnTrackballStopAction();
  return 1;
}

int 
Trackball3DInteractionMode
::OnMouseDrag(const FLTKEvent &event,
              const FLTKEvent &irisNotUsed(startEvent))
{
  m_Parent->OnTrackballDragAction(event.XCanvas[0],event.XCanvas[1]);
  return 1;
}

/**
 * \class Crosshair3DInteractionMode
 * \brief 3D interaction mode that takes care of 3D crosshair interaction
 *
 * \see Window3D
 */
class Crosshairs3DInteractionMode : public Window3D::EventHandler {
public:
  Crosshairs3DInteractionMode(Window3D *parent) : 
    Window3D::EventHandler(parent) {}

  int OnMousePress(const FLTKEvent &event);
};

int 
Crosshairs3DInteractionMode
::OnMousePress(const FLTKEvent &event)
{
  if(event.SoftButton == FL_LEFT_MOUSE)
    {
    m_Parent->OnCrosshairClickAction(event.XCanvas[0],event.XCanvas[1]);
    return 1;
    }
  else return 0;
}

/**
 * \class Scalpel3DInteractionMode
 * \brief 3D interaction mode that takes care of cutting 3D view in two
 *
 * \see Window3D
 */
class Scalpel3DInteractionMode : public Window3D::EventHandler {
public:
  Scalpel3DInteractionMode(Window3D *parent) : 
    Window3D::EventHandler(parent) 
  { 
    m_Started = false;
    m_Inside = false;
  }

  int OnMousePress(const FLTKEvent &event);
  int OnMouseMotion(const FLTKEvent &event);    
  int OnMouseEnter(const FLTKEvent &event);
  int OnMouseExit(const FLTKEvent &event);

  irisGetMacro(Started,bool);
  irisGetMacro(Inside,bool);
  irisGetMacro(StartPoint,Vector2i);
  irisGetMacro(EndPoint,Vector2i);

protected:
  bool m_Inside;
  bool m_Started;
  Vector2i m_StartPoint;
  Vector2i m_EndPoint;
};

int 
Scalpel3DInteractionMode
::OnMousePress(const FLTKEvent &event)
{
  if(!m_Started)
    {
    // Only for left button
    if(event.SoftButton != FL_LEFT_MOUSE) return 0;

    // Record the starting and ending points
    m_StartPoint = event.XCanvas;
    m_EndPoint = event.XCanvas;
    m_Started = true;
    m_Inside = true;
    }
  else
    {
    // This is the second click, after the user finished dragging
    m_EndPoint = event.XCanvas;
    m_Started = false;

    // If the user clicks another button, disengage, otherwise
    // draw the plane in the parent
    if(event.SoftButton == FL_LEFT_MOUSE)
      m_Parent->OnScalpelPointPairAction(
        m_StartPoint[0],m_StartPoint[1],m_EndPoint[0],m_EndPoint[1]);    
    }

  // Redraw the parent
  m_Canvas->redraw();

  // Eat the event
  return 1;
}

int 
Scalpel3DInteractionMode::
OnMouseMotion(const FLTKEvent &event)
{
  // Only valid if drawing has started
  if(!m_Started) return 0;
  
  // Record the end point
  m_EndPoint = event.XCanvas;

  // Redraw the parent
  m_Canvas->redraw();

  // Eat the event
  return 1;
}

int 
Scalpel3DInteractionMode
::OnMouseEnter(const FLTKEvent &event)
{
  // Only valid if drawing has started
  if(!m_Started) return 0;
  
  // Record that we're inside
  m_Inside = true;
  
  // Record the end point
  m_EndPoint = event.XCanvas;

  // Redraw the parent
  m_Canvas->redraw();

  // Eat the event
  return 1;
}

int 
Scalpel3DInteractionMode
::OnMouseExit(const FLTKEvent &irisNotUsed(event))
{
  // Only valid if drawing has started
  if(!m_Started) return 0;
  
  // Record that we're inside
  m_Inside = false;
  
  // Redraw the parent
  m_Canvas->redraw();

  // Eat the event
  return 1;
}


/**
 * \class Spraypaint3DInteractionMode
 * \brief 3D interaction mode that takes care of spraying on top of the 3D view
 *
 * \see Window3D
 */
class Spraypaint3DInteractionMode : public Window3D::EventHandler {
public:
  Spraypaint3DInteractionMode(Window3D *parent) : 
    Window3D::EventHandler(parent) {}

  int OnMousePress(const FLTKEvent &event);
  int OnMouseDrag(const FLTKEvent &event, const FLTKEvent &pressEvent);
};


int 
Spraypaint3DInteractionMode
::OnMousePress(const FLTKEvent &event)
{
  if(event.SoftButton == FL_LEFT_MOUSE)
    {
    m_Parent->OnSpraypaintClickAction(event.XCanvas[0],event.XCanvas[1]);
    return 1;
    }
  else return 0;
}

int 
Spraypaint3DInteractionMode
::OnMouseDrag(const FLTKEvent &event,
              const FLTKEvent &irisNotUsed(startEvent))
{
  if(event.SoftButton == FL_LEFT_MOUSE)
    {
    m_Parent->OnSpraypaintClickAction(event.XCanvas[0],event.XCanvas[1]);
    return 1;
    }
  else return 0;
}


Window3D
::Window3D(UserInterfaceBase *parentUI, FLTKCanvas *canvas)
: RecursiveInteractionMode(canvas)
{
  // Copy parent pointers
  m_ParentUI = parentUI;
  m_Driver = m_ParentUI->GetDriver();
  m_GlobalState = m_Driver->GetGlobalState();    

  // Pass parent pointer to the mesh object
  this->m_Mesh.Initialize(m_Driver);

  // Make sure FLTK canvas does not flip the Y coordinate
  m_Canvas->SetFlipYCoordinate(false);
  m_Canvas->SetGrabFocusOnEntry(true);
  
  // Clear the flags
  m_NeedsInitialization = 1;
  m_CursorVisible = 0;
  m_Mode = WIN3D_NONE;
  m_Plane.valid = -1;

  // Reset the vectors to zero
  m_WorldMatrix.set_identity();
  m_ImageSize.fill(0);
  m_Center.fill(0);
  m_DefaultHalf.fill(0);
  m_ViewHalf.fill(0);

  // Initialize the interaction modes
  m_CrosshairsMode = new Crosshairs3DInteractionMode(this); 
  m_TrackballMode = new Trackball3DInteractionMode(this);
  m_SpraypaintMode = new Spraypaint3DInteractionMode(this);
  m_ScalpelMode = new Scalpel3DInteractionMode(this);
  
  // Start with the trackball mode, which is prevailing
  PushInteractionMode(m_TrackballMode);
}


/** Enter the cross-hairs mode of operation */
void 
Window3D
::EnterCrosshairsMode()
{
  PopInteractionMode();
  PushInteractionMode(m_CrosshairsMode);
}

/** Enter the trackball mode of operation */
void 
Window3D
::EnterTrackballMode()
{
  PopInteractionMode();
  PushInteractionMode(m_TrackballMode);
}

/** Enter the scalpel mode of operation */
void 
Window3D
::EnterScalpelMode()
{
  PopInteractionMode();
  PushInteractionMode(m_ScalpelMode);
}

/** Enter the spraypaint mode of operation */
void 
Window3D
::EnterSpraypaintMode()
{
  PopInteractionMode();
  PushInteractionMode(m_SpraypaintMode);
}

Window3D
::~Window3D()
{
  delete m_CrosshairsMode;
  delete m_TrackballMode;
  delete m_SpraypaintMode;
  delete m_ScalpelMode;
}


/* Initializes lightning and other basic GL-state for the window */
void 
Window3D
::Initialize()
{
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glEnable( GL_DEPTH_TEST );

  // Set up the materials
  GLfloat light0Pos[4] = { 0.0, 0.0, 1.0, 0.0};
  GLfloat matAmb[4] = { 0.01, 0.01, 0.01, 1.00};
  GLfloat matDiff[4] = { 0.65, 0.65, 0.65, 1.00};
  GLfloat matSpec[4] = { 0.30, 0.30, 0.30, 1.00};
  GLfloat matShine = 10.0;
  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, matAmb);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, matDiff);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpec);
  glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, matShine);
  glEnable(GL_COLOR_MATERIAL);

  // Setup Lighting
  glLightfv(GL_LIGHT0, GL_POSITION, light0Pos);
  glEnable(GL_LIGHT0);
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
  glEnable(GL_LIGHTING);
}

void 
Window3D
::ClearScreen()
{
  // Hide the crosshairs.
  m_CursorVisible = 0;  

  // Hide the mesh.
  m_Mesh.Reset(); 
}

void 
Window3D
::ResetView()
{
  // Reset the trackball
  m_Trackball.Reset();

  // Rotate a little bit, so we see the three axes
  m_Trackball.StartRot(12,10,20,20);
  m_Trackball.TrackRot(10,8,20,20);
  m_Trackball.StopRot();

  if (m_Driver->GetCurrentImageData()->IsMainLoaded())
    {
    // data dimensions
    m_ImageSize = m_Driver->GetCurrentImageData()->GetVolumeExtents();  

    // world transform
    ResetWorldMatrix();

    // Volume size
    m_VolumeSize = vector_multiply_mixed<double,unsigned int,3>(
      m_Driver->GetCurrentImageData()->GetImageSpacing(),
      m_ImageSize);
    } 
  else
    {
    m_ImageSize.fill(0);
    m_WorldMatrix.set_identity();
    m_VolumeSize.fill(0.0);
    }

  // Compute the maximum extent of the image cube
  float xMaxDim = m_VolumeSize.max_value();
  
  m_DefaultHalf[X] = m_DefaultHalf[Y] = m_DefaultHalf[Z] = xMaxDim * 0.7 + 1.0;
  m_DefaultHalf[Z] *= 4.0;

  m_CursorVisible = 1;  // Show the crosshairs.
  m_Plane.valid = -1; // Resets the Cut m_Plane
  
  m_Samples.clear();

  // Fire 3D view update event
  m_ParentUI->OnTrackballUpdate();
}

void
Window3D
::ResetWorldMatrix()
{

  if (m_Driver->GetCurrentImageData()->IsMainLoaded())
    {

    // world transform
    m_WorldMatrix = m_Driver->GetCurrentImageData()->GetMain()->GetNiftiSform();

    }
}

void 
Window3D
::UpdateMesh(itk::Command *command)
{
  // make_current();
  try 
    {
    m_Mesh.GenerateMesh(command);
    }
  catch(vtkstd::bad_alloc &)
    {
    fl_alert("Out of memory error when generating 3D mesh.");
    }
  catch(IRISException & IRISexc)
    {
    fl_alert("%s", IRISexc.what());
    }
  m_Canvas->redraw();
}

void 
Window3D
::Accept()
{
  // Get the current drawing color
  unsigned char colorid = m_GlobalState->GetDrawingColorLabel();  

  // Apply the spraypaint samples to the image
  for(SampleListIterator it=m_Samples.begin();it!=m_Samples.end();++it)
    {
    m_Driver->GetCurrentImageData()->SetSegmentationVoxel(
      to_unsigned_int(*it), colorid );
    }
  
  // Clear the list of samples
  m_Samples.clear();
  
  // If the plane is valid, apply it for relabeling
  if (1 == m_Plane.valid )
    {   
    m_Driver->RelabelSegmentationWithCutPlane(m_Plane.vNormal, m_Plane.dIntercept);
    m_Plane.valid = -1;
    }
}

void
Window3D
::CheckErrors()
{
  GLenum error;
  while ((error = glGetError()) != GL_NO_ERROR)
    {
    std::cerr << "GL-Error: " << (char *) gluErrorString(error) << std::endl;
    }
}

void 
Window3D
::OnDraw()
{
  // Respond to a resize if necessary
  if (!m_Canvas->valid())
    {
    Initialize();
    glViewport(0,0,m_Canvas->w(),m_Canvas->h());
    }
  
  // Initialize GL if necessary
  if(m_NeedsInitialization)
    {
    ResetView();
    Initialize();
    m_NeedsInitialization = false;
    }      

  // Get the properties for the background color
  Vector3d clrBack = 
    m_ParentUI->GetAppearanceSettings()->GetUIElement(
    SNAPAppearanceSettings::BACKGROUND_3D).NormalColor;

  glClearColor(clrBack[0],clrBack[1],clrBack[2],1);
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

  // Compute the center of rotation
  m_CenterOfRotation = 
    affine_transform_point(m_WorldMatrix, m_Driver->GetCursorPosition());

  // Set up the projection matrix
  SetupProjection();

  // Set up the model view matrix
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();  
  glLoadIdentity();

  // Update the screen geometry
  glTranslatef( m_Trackball.GetPanX(), m_Trackball.GetPanY(), 0.0 );
  glMultMatrixf( m_Trackball.GetRot() );
  glTranslate( - m_CenterOfRotation );

  // Apply the world matrix - all subsequent ops are in voxel space
  glPushMatrix();
  glMultMatrixd(m_WorldMatrix.transpose().data_block());
  // glTranslate(m_Origin);
  // glScale(m_Spacing);

  // Draw things in pixel coords
  DrawCrosshairs();
  DrawSamples();
  
  // Undo world matrix - back to drawing in native space
  glPopMatrix();

  // Draw the cut plane
  DrawCutPlane();

  // The mesh is already in isotropic coords (needed for marching cubes)
  m_Mesh.Display();

  // Restore the matrix state
  glPopMatrix(); 

  // Draw any overlays there may be
  if(m_ScalpelMode->GetStarted() && m_ScalpelMode->GetInside())
    {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0,m_Canvas->w(),m_Canvas->h(),0);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glPushAttrib(GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_ALWAYS);

    // Get the points
    Vector2d x1 = to_double(m_ScalpelMode->GetStartPoint());
    Vector2d x2 = to_double(m_ScalpelMode->GetEndPoint());
    Vector2d d = x2-x1;

    // Draw a line between the two points
    glColor3d(1,1,1);
    glBegin(GL_LINES);
    glVertex2d(x1[0],x1[1]);    
    glVertex2d(x2[0],x2[1]);    
    glEnd();

    // If the points are far enough apart, draw the normal
    if(d.two_norm() > 10)
      {
      // Draw the normal midway through the line
      Vector2d n = Vector2d(-d[1],d[0]).normalize();
      Vector2d p1 = 0.5 * (x1 + x2);
      Vector2d p2 = p1 + 10.0 * n;
      
      glBegin(GL_LINES);
      glVertex2d(p1[0],p1[1]);    
      glVertex2d(p2[0],p2[1]);    
      glEnd();

      // Draw a colored triangle
      d.normalize();
      Vector2d u1 = p2 + 4.0 * d;
      Vector2d u2 = p2 - 4.0 * d;
      Vector2d u3 = p2 + 1.732 * 4.0 * n;

      glBegin(GL_TRIANGLES);
      glVertex2d(u1[0],u1[1]);    
      glVertex2d(u2[0],u2[1]);    
      glVertex2d(u3[0],u3[1]);    
      glEnd();
      
      }
    
    glPopMatrix();
    
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    
    glPopAttrib();
  }

  glFlush();
  // CheckErrors();
}

void 
Window3D
::OnRotateStartAction(int x, int y)
{
  if ( m_Mode != WIN3D_NONE ) return;
  m_Mode = WIN3D_ROTATE;
  m_Trackball.StartRot(x, y, m_Canvas->w(), m_Canvas->h());
  m_ParentUI->OnTrackballUpdate();
}

void 
Window3D
::OnPanStartAction(int x, int y)
{
  if ( m_Mode != WIN3D_NONE ) return;
  m_Mode = WIN3D_PAN;
  m_Trackball.StartPan(x, y);
  m_ParentUI->OnTrackballUpdate();
}

void 
Window3D
::OnZoomStartAction(int irisNotUsed(x), int y)
{
  if ( m_Mode != WIN3D_NONE ) return;
  m_Mode = WIN3D_ZOOM;
  m_Trackball.StartZoom(y);
  m_ParentUI->OnTrackballUpdate();
}

void 
Window3D
::OnTrackballDragAction(int x, int y)
{
  switch (m_Mode)
    {
    case WIN3D_ROTATE: 
      m_Trackball.TrackRot(x, y, m_Canvas->w(), m_Canvas->h()); 
      break;
    case WIN3D_ZOOM:   
      m_Trackball.TrackZoom(y); 
      // this->SetupProjection();
      break;
    case WIN3D_PAN:    
      m_Trackball.TrackPan(x, y, m_Canvas->w(), m_Canvas->h(),
        2 * m_ViewHalf[X], 2 * m_ViewHalf[Y]);
      break;
    default: break;
    }

  m_ParentUI->OnTrackballUpdate();
  m_Canvas->redraw();
}

void 
Window3D
::OnTrackballStopAction()
{
  switch (m_Mode)
    {
    case WIN3D_ROTATE:  m_Trackball.StopRot(); break;
    case WIN3D_PAN:     m_Trackball.StopPan(); break;
    case WIN3D_ZOOM:    
      m_Trackball.StopZoom(); 
      // this->SetupProjection();
      break;
    default: break;
    }
  m_Mode = WIN3D_NONE;
  m_ParentUI->OnTrackballUpdate();
}

void 
Window3D
::OnCrosshairClickAction(int x,int y)
{
  // Make sure that there is a valid image
  if (!m_Driver->GetCurrentImageData()->IsMainLoaded()) return;

  // Only respond to the left mouse button (why?)
  Vector3i hit;
  if (IntersectSegData(x, y, hit))
    {
    m_Driver->SetCursorPosition(to_unsigned_int(hit));
    }

  m_ParentUI->OnCrosshairPositionUpdate();
  m_ParentUI->RedrawWindows();
}

void 
Window3D
::OnSpraypaintClickAction(int x,int y)
{
  // Make sure that there is a valid image
  if (!m_Driver->GetCurrentImageData()->IsMainLoaded()) return;

  Vector3i hit;
  if (this->IntersectSegData(x, y, hit))
    {
    AddSample( hit );
    m_ParentUI->OnIRISMeshEditingAction();
    m_Canvas->redraw();
    }
}

void 
Window3D
::OnScalpelPointPairAction(int x1, int y1, int x2, int y2)
{
  // Requires a loaded image
  if (!m_Driver->GetCurrentImageData()->IsMainLoaded()) return;

  // Pass in the first point
  // OnCutPlanePointRayAction(x1, y1, 1);
  // OnCutPlanePointRayAction(x2, y2, 2);

  if(ComputeCutPlane(x1,y1,x2,y2)) 
    {
    m_Plane.valid = 1;
    m_ParentUI->OnIRISMeshEditingAction();
    }
        
  m_Canvas->redraw();
}

/**
  Does translation and rotation from the m_Trackball (not anisotropic scaling).
  Make sure to glPopMatrix() after use!
  */
void 
Window3D
::SetupModelView()
{
}

void 
Window3D
::SetupProjection()
{
  // Get the view extent 'radius'
  m_ViewHalf = m_DefaultHalf / (double) m_Trackball.GetZoom();
  
  double x, y;
  if(m_ViewHalf[X] * m_Canvas->h() > m_ViewHalf[Y] * m_Canvas->w())
    {
    x = m_ViewHalf[X]; 
    y = m_Canvas->h() * x / m_Canvas->w();
    }
  else
    {
    y = m_ViewHalf[Y]; 
    x = m_Canvas->w() * y / m_Canvas->h();
    }

  // Set up the coordinate projection
  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  glOrtho(-x, x, -y, y, -m_DefaultHalf[Z], m_DefaultHalf[Z]);
}

// Get a copy of the viewport/m_Modelview/projection matrices OpenGL uses
void Window3D::ComputeMatricies( GLint *vport, double *mview, double *proj )
{
  // Compute the center of rotation
  m_CenterOfRotation = 
    affine_transform_point(m_WorldMatrix, m_Driver->GetCursorPosition());

  // Set up the model view matrix
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();  
  glLoadIdentity();

  // Update the screen geometry
  glTranslatef( m_Trackball.GetPanX(), m_Trackball.GetPanY(), 0.0 );
  glMultMatrixf( m_Trackball.GetRot() );
  glTranslate( -m_CenterOfRotation );
  glMultMatrixd(m_WorldMatrix.transpose().data_block());

  glGetIntegerv( GL_VIEWPORT, vport );
  glGetDoublev( GL_MODELVIEW_MATRIX, mview );
  glGetDoublev( GL_PROJECTION_MATRIX, proj );

  glPopMatrix();
}

void Window3D::ComputeRay( int x, int y, double *mvmatrix, double *projmatrix,
                           GLint *viewport, Vector3d &v, Vector3d &r )
{
  int val;
  val = gluUnProject( (GLdouble) x, (GLdouble) y, 0.0,
                      mvmatrix, projmatrix, viewport,
                      &(v[0]), &(v[1]), &(v[2]) );
  if ( val == GL_FALSE ) std::cerr << "gluUnProject #1 FAILED!!!!!" << std::endl;
  
  val = gluUnProject( (GLdouble) x, (GLdouble) y, 1.0,
                      mvmatrix, projmatrix, viewport,
                      &(r[0]), &(r[1]), &(r[2]) );
  if ( val == GL_FALSE ) std::cerr << "gluUnProject #2 FAILED!!!!!" << std::endl;

  r[0] = r[0] - v[0];
  r[1] = r[1] - v[1];
  r[2] = r[2] - v[2];
}

bool 
Window3D
::ComputeCutPlane(int wx1, int wy1, int wx2, int wy2)
{
  // Open GL matrices
  double mvmatrix[16];
  double projmatrix[16];
  GLint viewport[4];
  Vector3d x1,x2,p1,p2;

  // Compute the GL matrices
  ComputeMatricies( viewport, mvmatrix, projmatrix );

  // Flip the y coordinate
  wy1 = viewport[3] - wy1 - 1;
  wy2 = viewport[3] - wy2 - 1;

  // Compute the normal to the viewplane  
  gluUnProject(0,0,0,mvmatrix,projmatrix,viewport,
               p1.data_block(),p1.data_block()+1,p1.data_block()+2);
  gluUnProject(0,0,1,mvmatrix,projmatrix,viewport,
               p2.data_block(),p2.data_block()+1,p2.data_block()+2);
  
  // W is a vector pointing into the screen
  Vector3d w = p2 - p1;

  // Compute the vector connecting the two points currently lying on the
  // view plane
  gluUnProject(wx1,wy1,0,mvmatrix,projmatrix,viewport,
               x1.data_block(),x1.data_block()+1,x1.data_block()+2);
  gluUnProject(wx2,wy2,0,mvmatrix,projmatrix,viewport,
               x2.data_block(),x2.data_block()+1,x2.data_block()+2);

  // Now we have two orthogonal vectors laying on the cut plane.  All we have
  // to do is take the cross product
  Vector3d delta = x2-x1;
  Vector3d n = - itk_cross_3d(delta, w);

  // Compute the length of the normal and exit if it's zero
  double l = n.two_norm();
  if(l == 0.0) return false;

  
  // Compute the distance to the origin
  m_Plane.vNormal = n.normalize();

  // Check if the normal requires flipping (if the Jacobian of the world matrix
  // is negative)
  if(vnl_det(m_WorldMatrix) < 0)
    m_Plane.vNormal = -m_Plane.vNormal;

  m_Plane.dIntercept = dot_product(x1,m_Plane.vNormal);

  // Now, this is not enough, because we want to be able to draw the plane
  // in space.  In order to do this we need four corners of a square on the
  // plane.

  // Compute the points on the plane in world space
  
  Vector3d x1World = affine_transform_point(m_WorldMatrix, x1);
  Vector3d x2World = affine_transform_point(m_WorldMatrix, x2);
  Vector3d p1World = affine_transform_point(m_WorldMatrix, p1);
  Vector3d p2World = affine_transform_point(m_WorldMatrix, p2);
  
  // Compute the normal in world coordinates
  Vector3d nWorld = - itk_cross_3d((vnl_vector<double>) (x2World - x1World),
                                   (vnl_vector<double>) (p2World - p1World));
  m_Plane.vNormalWorld = nWorld.normalize();

  // Compute the intercept in world coordinates  
  double interceptWorld = dot_product(x1World,m_Plane.vNormalWorld);

  // Compute the center of the volume
  Vector3d xVol = to_double(m_VolumeSize) * 0.5;

  // Compute the center of the volume in world coordinates
  Vector3d xVolCenter = 
    affine_transform_point(m_WorldMatrix, to_double(m_ImageSize) * 0.5);
  
  // Use that to compute the center of the square
  double edgeLength = (xVol[0] > xVol[1]) ? xVol[0] : xVol[1];
  edgeLength = (edgeLength > xVol[2]) ? edgeLength : xVol[2];

  // Now compute the center point of the square   
  Vector3d m_Origin = m_Driver->GetCurrentImageData()->GetImageOrigin();
  Vector3d xCenter = 
    xVolCenter - m_Plane.vNormalWorld *
     (dot_product(xVolCenter,m_Plane.vNormalWorld) - interceptWorld);
  
  // Compute the 'up' vector and the 'in' vector
  Vector3d vUp = (x2World - x1World).normalize();
  Vector3d vIn = (p2World - p1World).normalize();

  // Compute the corners
  m_Plane.xDisplayCorner[0] = xCenter + edgeLength * (vUp + vIn);
  m_Plane.xDisplayCorner[1] = xCenter + edgeLength * (vUp - vIn);
  m_Plane.xDisplayCorner[2] = xCenter + edgeLength * (- vUp - vIn);
  m_Plane.xDisplayCorner[3] = xCenter + edgeLength * (- vUp + vIn);

  return true;
}



//------------------------------------------------------------------------
// IntersectSegData(int mouse_x, int mouse_y, Vector3i *hit)
// computes a m_Ray going straight back from the current viewpoint
// from the mouse click position on screen.
// The output Vector3i hit is in image coords.
//
//------------------------------------------------------------------------
int Window3D::IntersectSegData(int mouse_x, int mouse_y, Vector3i &hit)
{
  double mvmatrix[16];
  double projmatrix[16];
  GLint viewport[4];

  m_Canvas->make_current(); // update GL state
  ComputeMatricies( viewport, mvmatrix, projmatrix );
  int x = mouse_x;
  int y = viewport[3] - mouse_y - 1;
  ComputeRay( x, y, mvmatrix, projmatrix, viewport, m_Point, m_Ray );

  // The result 
  int result = 0;
  
  // Depending on the situation, we may intersect with a snake image or a
  // segmentation image
  // TODO: Need both conditions?
  if(m_GlobalState->GetSnakeActive() && 
     m_Driver->GetSNAPImageData()->IsSnakeLoaded())
    {
    typedef ImageRayIntersectionFinder<
      float,SnakeImageHitTester> RayCasterType;

    RayCasterType caster;

    result = 
      caster.FindIntersection(
        m_Driver->GetSNAPImageData()->GetLevelSetImage(),
        m_Point,m_Ray,hit);
    }
  else
    {
    typedef ImageRayIntersectionFinder<
      LabelType,LabelImageHitTester> RayCasterType;

    RayCasterType caster;
    caster.SetHitTester(LabelImageHitTester(m_Driver->GetColorLabelTable()));
    result = 
      caster.FindIntersection(
        m_Driver->GetCurrentImageData()->GetSegmentation()->GetImage(),
        m_Point,m_Ray,hit);
    }
  
  // m_Ray now has the proj m_Ray, m_Point is the m_Point in image space
  switch (result)
    {
    case 1: return 1;
    case -1: /*std::cerr << "RAY WAS INVALID!" << std::endl;*/ break;
    /* default: std::cerr << "No hit found" << std::endl;*/
    }
  return 0;
}

void Window3D::AddSample( Vector3i s )
{
  // Add another sample.
  m_Samples.push_back(s);
}

void DrawCube( Vector3f &x, Vector3f &y )
  {
  glBegin(GL_LINE_LOOP);
  glVertex3f( x[0], x[1], x[2] );
  glVertex3f( x[0], y[1], x[2] );
  glVertex3f( x[0], y[1], y[2] );
  glVertex3f( x[0], x[1], y[2] );
  glEnd();

  glBegin(GL_LINE_LOOP);
  glVertex3f( y[0], x[1], x[2] );
  glVertex3f( y[0], y[1], x[2] );
  glVertex3f( y[0], y[1], y[2] );
  glVertex3f( y[0], x[1], y[2] );
  glEnd();

  glBegin(GL_LINES);
  glVertex3f( x[0], x[1], x[2] );
  glVertex3f( y[0], x[1], x[2] );
  glVertex3f( x[0], y[1], x[2] );
  glVertex3f( y[0], y[1], x[2] );
  glVertex3f( x[0], x[1], y[2] );
  glVertex3f( y[0], x[1], y[2] );
  glVertex3f( x[0], y[1], y[2] );
  glVertex3f( y[0], y[1], y[2] );
  glEnd();
  }

void DrawCoordinateCutPlane( unsigned int iPlane, Vector3f &a, Vector3f &b, Vector3f &x )
  {
  glBegin(GL_LINE_LOOP);

  switch(iPlane) {
    case 0 :
      glVertex3f( x[0], a[1], a[2] );
      glVertex3f( x[0], a[1], b[2] );
      glVertex3f( x[0], b[1], b[2] );
      glVertex3f( x[0], b[1], a[2] );
      break;

    case 1 :
      glVertex3f( a[0], x[1], a[2] );
      glVertex3f( a[0], x[1], b[2] );
      glVertex3f( b[0], x[1], b[2] );
      glVertex3f( b[0], x[1], a[2] );
      break;

    case 2 :
      glVertex3f( a[0], a[1], x[2] );
      glVertex3f( a[0], b[1], x[2] );
      glVertex3f( b[0], b[1], x[2] );
      glVertex3f( b[0], a[1], x[2] );
      break;
    }

  glEnd();
  }

void Window3D::DrawCrosshairs()
{
  if ( !m_CursorVisible ) return;

  // Set up the GL state
  glPushAttrib(GL_LINE_BIT | GL_LIGHTING_BIT | GL_COLOR_BUFFER_BIT);
  glDisable(GL_LIGHTING);

  // Get the crosshair position
  Vector3f xCross = 
    to_float(m_Driver->GetCursorPosition()) + Vector3f(0.5f);

  // Get the UI element properties for the image box
  SNAPAppearanceSettings::Element &eltBox = 
    m_ParentUI->GetAppearanceSettings()->GetUIElement(
    SNAPAppearanceSettings::IMAGE_BOX_3D);

  // Draw the image box
  if(eltBox.Visible)
    {
    // Set the line properties
    SNAPAppearanceSettings::ApplyUIElementLineSettings(eltBox);

    // The cube around the image
    glColor3dv(eltBox.NormalColor.data_block());
    Vector3f zeros(0.0f);
    Vector3f imageSize = to_float(m_ImageSize);
    DrawCube( zeros, imageSize );

    // The slice planes
    glColor3dv(eltBox.ActiveColor.data_block());
    for(int d = 0; d < 3; d++)
      DrawCoordinateCutPlane(d, zeros, imageSize, xCross );
    }

  // Get the UI element properties for the crosshairs
  SNAPAppearanceSettings::Element &eltCross = 
    m_ParentUI->GetAppearanceSettings()->GetUIElement(
    SNAPAppearanceSettings::CROSSHAIRS_3D);

  // Exit if crosshairs are to be ignored
  if(eltCross.Visible) 
    {
    // Set up the line properties
    glColor3dv(eltCross.NormalColor.data_block());
    SNAPAppearanceSettings::ApplyUIElementLineSettings(eltCross);

    // Draw the lines
    glBegin( GL_LINES );
    for (int i=0; i<3; i++)
      {
      float end1[3], end2[3];
      for (int j=0; j<3; j++)
        end1[j] = end2[j] = xCross[j];
      // end1[i] = m_Center[i] - m_ImageSize[i]*0.7+1;
      // end2[i] = m_Center[i] + m_ImageSize[i]*0.7+1;
      end1[i] = 0.0f;
      end2[i] = m_ImageSize[i];

      glVertex3fv(end1);
      glVertex3fv(end2);
      }
    glEnd();
    }

#if DEBUGGING
  glColor3f( 1.0, 1.0, 0.0 );
  glBegin( GL_LINES );
  glVertex3d( m_Point[0],        m_Point[1],        m_Point[2] );
  glVertex3d( m_Point[0]+m_Ray[0], m_Point[1]+m_Ray[1], m_Point[2]+m_Ray[2] );
  glEnd();

  std::cerr << "A = ( " << m_Point[0] << ", " << m_Point[1] << ", " << m_Point[2] << " )" << std::endl;
  std::cerr << "B = ( " << m_Point[0]+m_Ray[0]
  << ", " << m_Point[1]+m_Ray[1]
  << ", " << m_Point[2]+m_Ray[2] << " )" << std::endl;
#endif

  // Finish
  glPopAttrib();
}

void Window3D::DrawSamples()
{
  unsigned char index = m_GlobalState->GetDrawingColorLabel();
  unsigned char rgb[3];
  m_Driver->GetColorLabelTable()->GetColorLabel(index).GetRGBVector(rgb);

  glColor3ubv(rgb);
  for (SampleListIterator it=m_Samples.begin();it!=m_Samples.end();it++)
    {
    glPushMatrix(); 
    glTranslate(to_float(*it));

    GLUquadric *quad = gluNewQuadric();
    gluSphere(quad,1.0,4,4);
    gluDeleteQuadric(quad);
    // glutSolidSphere( 1.0, 4, 4 );

    glPopMatrix();
    }
}

//-----------------------------------------------------------------------
// DrawCutPlane
//   Provides User Feedback Information when in 3D Window Mode
//   when a Cutplane is defined.
// Input: the 'plane' member
// Output: GLplane representing the Cutplane that is defined by the user
//-----------------------------------------------------------------------
void Window3D
::DrawCutPlane() 
{
  if (m_Plane.valid != 1) return;

  Vector3d corner[4];  
  corner[0] = m_Plane.xDisplayCorner[0];
  corner[1] = m_Plane.xDisplayCorner[1];
  corner[2] = m_Plane.xDisplayCorner[2];
  corner[3] = m_Plane.xDisplayCorner[3];

  // Use the current label color
  unsigned char rgb[3];
  m_Driver->GetColorLabelTable()->GetColorLabel(
    m_GlobalState->GetDrawingColorLabel()).GetRGBVector(rgb);

  // Save the settings
  glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT);

  // Create a stipple pattern
  GLubyte stipple[128];
  for(unsigned int q = 0; q < 128; q++)
    stipple[q] = ((q >> 2) % 2 == 0) ? 0xAA : 0x55;

  // Draw a semi-transparent quad
  glEnable(GL_POLYGON_STIPPLE);
  glPolygonStipple(stipple);
  glColor3ubv(rgb);
  glBegin(GL_QUADS);
  glVertex(corner[0]);
  glVertex(corner[1]);
  glVertex(corner[2]);
  glVertex(corner[3]);
  glEnd();
  glDisable(GL_POLYGON_STIPPLE);

  // Draw the plane using lines
  glEnable(GL_LINE_SMOOTH);
  glEnable(GL_LINE_STIPPLE);
  glEnable(GL_BLEND);
  glDisable(GL_LIGHTING);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glLineWidth(2.0);
  glLineStipple(2,0xAAAA);

  
  // Start with a white color
  glColor3d(1,1,1);
  Vector3d planeUnitNormal = Vector3d(m_Plane.vNormalWorld).normalize();
  for(unsigned int i=0;i<4;i++)
    {
    glPushMatrix();

    // Create a parallel plane to create a moving effect
    Vector3d vOffset = planeUnitNormal * (1.0 * i);
    glTranslate(vOffset);

    // Draw a line loop
    glBegin(GL_LINE_LOOP);
    glVertex(corner[0]);
    glVertex(corner[1]);
    glVertex(corner[2]);
    glVertex(corner[3]);
    glEnd();

    // Switch to new color
    glColor3ubv(rgb);

    glPopMatrix();
    }

  glPopAttrib();
};

int 
Window3D
::OnKeyAction(int key)
{
  if(Fl::event_state() != FL_CTRL && key == 's')
    {
    // Store the state of the trackball
    m_TrackballBackup = m_Trackball;
    return 1;
    }
  else if(Fl::event_state() != FL_CTRL && key == 'r')
    {
    // Restore the trackball state
    m_Trackball = m_TrackballBackup;
    m_Canvas->redraw();
    m_ParentUI->OnTrackballUpdate();
    return 1;
    }
  return 0;
}

/*
 *$Log: Window3D.cxx,v $
 *Revision 1.13  2009/10/26 16:40:19  pyushkevich
 *FIX(2039124): spray paint was not respecting hidden labels. Also update mesh was not on after label vis change
 *
 *Revision 1.12  2009/10/26 07:34:11  pyushkevich
 *ENH: substantially reduced memory footprint when loading float NIFTI images
 *
 *Revision 1.11  2009/10/25 13:17:05  pyushkevich
 *FIX: bugs in SF.net, crash on mesh update in large images, bad vols/stats output
 *
 *Revision 1.10  2009/07/16 22:02:29  pyushkevich
 *Made OpenGLTexture non-templated
 *
 *Revision 1.9  2009/06/02 04:32:46  garyhuizhang
 *ENH: layer support
 *
 *Revision 1.8  2009/01/30 23:08:21  garyhuizhang
 *ENH: better implementation of the keyboard shortcuts that do not require the SHIFT key
 *
 *Revision 1.7  2009/01/30 20:57:34  garyhuizhang
 *Bug Fix: check if any of the CTRL keys is pressed in Windows3D::OnKeyAction()
 *
 *Revision 1.6  2009/01/23 20:54:11  pyushkevich
 *FIX: fixed cut plane behavior, which was broken by earlier 3D changes
 *
 *Revision 1.5  2009/01/23 20:09:38  pyushkevich
 *FIX: 3D rendering now takes place in Nifti(RAS) world coordinates, rather than the VTK (x spacing + origin) coordinates. As part of this, itk::OrientedImage is now used for 3D images in SNAP. Still have to fix cut plane code in Window3D
 *
 *Revision 1.4  2009/01/17 10:40:28  pyushkevich
 *Added synchronization to 3D window viewpoint
 *
 *Revision 1.3  2008/02/10 23:55:22  pyushkevich
 *Added "Auto" button to the intensity curve window; Added prompt before quitting on unsaved data; Fixed issues with undo on segmentation image load; Added synchronization between SNAP sessions.
 *
 *Revision 1.2  2007/12/30 04:05:29  pyushkevich
 *GPL License
 *
 *Revision 1.1  2006/12/02 04:22:27  pyushkevich
 *Initial sf checkin
 *
 *Revision 1.1.1.1  2006/09/26 23:56:17  pauly2
 *Import
 *
 *Revision 1.33  2006/02/01 20:21:27  pauly
 *ENH: An improvement to the main SNAP UI structure: one set of GL windows is used to support SNAP and IRIS modes
 *
 *Revision 1.32  2006/01/05 18:03:09  pauly
 *STYLE: Removed unnecessary console messages from SNAP
 *
 *Revision 1.31  2005/12/12 00:27:45  pauly
 *ENH: Preparing SNAP for 1.4 release. Snapshot functionality
 *
 *Revision 1.30  2005/11/23 14:32:15  ibanez
 *BUG: 2404. Patch provided by Paul Yushkevish.
 *
 *Revision 1.29  2005/10/29 14:00:15  pauly
 *ENH: SNAP enhacements like color maps and progress bar for 3D rendering
 *
 *Revision 1.28  2005/08/10 03:24:21  pauly
 *BUG: Corrected problems with 3D window, label IO from association files
 *
 *Revision 1.27  2005/04/21 14:46:30  pauly
 *ENH: Improved management and editing of color labels in SNAP
 *
 *Revision 1.26  2005/03/08 03:12:51  pauly
 *BUG: Minor bugfixes in SNAP, mostly to the user interface
 *
 *Revision 1.25  2004/12/31 17:34:04  lorensen
 *COMP: gcc3.4 issues.
 *
 *Revision 1.24  2004/10/04 17:41:46  pauly
 *FIX: Filename extensions for FLTK includes
 *
 *Revision 1.23  2004/09/21 16:13:35  jjomier
 *FIX: Linux, gcc3.3 fixes
 *
 *Revision 1.22  2004/09/21 15:50:51  jjomier
 *FIX: vector_multiply_mixed requires template parameters otherwise MSVC cannot deduce them
 *
 *Revision 1.21  2004/09/14 14:11:11  pauly
 *ENH: Added an activation manager to main UI class, improved snake code, various UI fixes and additions
 *
 *Revision 1.20  2004/08/26 18:29:20  pauly
 *ENH: New user interface for configuring the UI options
 *
 *Revision 1.19  2004/07/29 14:02:05  pauly
 *ENH: An interface for changing SNAP appearance settings
 *
 *Revision 1.18  2004/07/22 19:22:51  pauly
 *ENH: Large image support for SNAP. This includes being able to use more screen real estate to display a slice, a fix to the bug with manual segmentation of images larger than the window size, and a thumbnail used when zooming into the image.
 *
 *Revision 1.17  2004/06/01 13:33:55  king
 *ERR: Fix for cross_3d to work with both the ITK version and current cvs version of vxl.
 *
 *Revision 1.16  2004/05/12 18:09:12  pauly
 *FIX:Error with cross_3d symbol
 *
 *Revision 1.15  2004/01/27 18:18:44  pauly
 *FIX: Last MAC OSX fix
 *
 *Revision 1.14  2004/01/27 18:05:38  pauly
 *FIX: More MAC OSX fixes. Also removed old snake code no longer in use
 *
 *Revision 1.13  2004/01/27 17:34:00  pauly
 *FIX: Compiling on Mac OSX, issue with GLU include file
 *
 *Revision 1.12  2003/11/25 23:32:48  pauly
 *FIX: Snake evolution did not work in multiprocessor mode
 *
 *Revision 1.11  2003/11/10 00:24:50  pauly
 **** empty log message ***
 *
 *Revision 1.10  2003/10/14 13:44:27  pauly
 *FIX: Fixed warnings on gcc-3.3
 *
 *Revision 1.9  2003/10/10 15:04:21  pauly
 *ENH: Cut plane improvements (paint-over-visible and paint-over-one)
 *
 *Revision 1.8  2003/10/10 14:25:55  pauly
 *FIX: Ensured that code compiles on gcc 3-3
 *
 *Revision 1.7  2003/10/09 22:45:15  pauly
 *EMH: Improvements in 3D functionality and snake parameter preview
 *
 *Revision 1.6  2003/10/02 20:57:46  pauly
 *FIX: Made sure that the previous check-in compiles on Linux
 *
 *Revision 1.5  2003/10/02 14:55:53  pauly
 *ENH: Development during the September code freeze
 *
 *Revision 1.2  2003/09/11 19:23:29  pauly
 *FIX: Code compiles and runs on UNIX platform
 *
 *Revision 1.1  2003/09/11 13:51:15  pauly
 *FIX: Enabled loading of images with different orientations
 *ENH: Implemented image save and load operations
 *
 *Revision 1.4  2003/08/28 22:58:30  pauly
 *FIX: Erratic scrollbar behavior
 *
 *Revision 1.3  2003/08/27 14:03:24  pauly
 *FIX: Made sure that -Wall option in gcc generates 0 warnings.
 *FIX: Removed 'comment within comment' problem in the cvs log.
 *
 *Revision 1.2  2003/08/27 04:57:47  pauly
 *FIX: A large number of bugs has been fixed for 1.4 release
 *
 *Revision 1.1  2003/07/12 04:46:51  pauly
 *Initial checkin of the SNAP application into the InsightApplications tree.
 *
 *Revision 1.2  2003/07/12 01:34:18  pauly
 *More final changes before ITK checkin
 *
 *Revision 1.1  2003/07/11 23:25:33  pauly
 **** empty log message ***
 *
 *Revision 1.9  2003/06/08 23:27:56  pauly
 *Changed variable names using combination of ctags, egrep, and perl.
 *
 *Revision 1.8  2003/06/04 04:52:17  pauly
 *More UI fixes for the demo
 *
 *Revision 1.7  2003/05/08 21:59:05  pauly
 *SNAP is almost working
 *
 *Revision 1.6  2003/05/07 19:14:46  pauly
 *More progress on getting old segmentation working in the new SNAP.  Almost there, region of interest and bubbles are working.
 *
 *Revision 1.5  2003/04/29 14:01:42  pauly
 *Charlotte Trip
 *
 *Revision 1.4  2003/04/23 06:05:18  pauly
 **** empty log message ***
 *
 *Revision 1.3  2003/04/18 17:32:18  pauly
 **** empty log message ***
 *
 *Revision 1.2  2003/04/16 05:04:17  pauly
 *Incorporated intensity modification into the snap pipeline
 *New IRISApplication
 *Random goodies
 *
 *Revision 1.1  2003/03/07 19:29:48  pauly
 *Initial checkin
 *
 *Revision 1.2  2002/12/16 16:40:19  pauly
 **** empty log message ***
 *
 *Revision 1.1.1.1  2002/12/10 01:35:36  pauly
 *Started the project repository
 *
 *
 *Revision 1.10  2002/06/04 20:26:23  seanho
 *new rotation code from iris
 *
 *Revision 1.9  2002/04/27 18:31:05  moon
 *Finished commenting
 *
 *Revision 1.8  2002/04/24 17:15:13  bobkov
 *made no changes
 *
 *Revision 1.7  2002/04/23 22:00:39  moon
 *Just put in a couple glMatrixMode(GL_MODELVIEW) in a few places I thought it should
 *be just to be cautious.  I don't think it changed anything.
 *
 *Revision 1.6  2002/04/22 21:56:21  moon
 *Put in code to get crosshairs m_Mode working in 3D window.  Just checks global
 *flag in the mouseclick method to see if we're in snake m_Mode, so that the
 *right windows get redrawn.
 *
 *Revision 1.5  2002/04/13 16:22:40  moon
 *Fixed the problem with the 3D window drawing in black.
 *The draw method needed to call Init() in the if (!valid()) block.  It had two
 *checks, for !valid, and m_NeedsInitialization.  They just needed to be combined so that
 *lighting, etc. was set up either way.  The bug seems to be fixed.
 *
 *Revision 1.4  2002/04/10 21:22:12  moon
 *added some make_current calls to some methods, which seems to help the window to
 *update better, but the color/lighting problem is still there.
 *
 *Revision 1.3  2002/04/10 20:26:24  moon
 *just put in some debug statements.  Trying to debug the 3dwindow not drawing right.
 *
 *Revision 1.2  2002/03/08 14:06:32  moon
 *Added Header and Log tags to all files
 **/
