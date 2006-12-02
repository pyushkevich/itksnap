/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: PaintbrushInteractionMode.cxx,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:27 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#include "PaintbrushInteractionMode.h"

#include "GlobalState.h"
#include "PolygonDrawing.h"
#include "UserInterfaceBase.h"
#include "IRISApplication.h"
#include "SNAPAppearanceSettings.h"
#include <algorithm>

using namespace std;


PaintbrushInteractionMode
::PaintbrushInteractionMode(GenericSliceWindow *parent)
: GenericSliceWindow::EventHandler(parent)
{
}

PaintbrushInteractionMode
::~PaintbrushInteractionMode()
{
}

void 
PaintbrushInteractionMode::
BuildBrush(const PaintbrushSettings &ps)
{
  // The sides of the box
  double s0 = 0.5 - ps.radius;

  // The square of the radius
  double r2 = ps.radius * ps.radius;

  // The range of the outline edges
  int z0 = (int) ceil(ps.radius);

  // The boundary of the paintbrush is a set of between-pixel edges that
  // separate the pixels inside the brush from the pixels outside of the
  // brush.
  m_MaskEdges.clear();
  for(int z = -z0; z < z0; z++)
    {
    double x = std::min(z, z+1);
    double y = (ps.shape == PAINTBRUSH_ROUND) 
      ? ceil(sqrt(r2 - x * x)) 
      : ceil(r2);
    m_MaskEdges.push_back(make_pair(Vector2d(z, y), Vector2d(z+1,y)));
    // m_MaskEdges.push_back(make_pair(Vector2d(z, -y), Vector2d(z+1,-y)));
    // m_MaskEdges.push_back(make_pair(Vector2d(y, z), Vector2d(y,z+1)));
    // m_MaskEdges.push_back(make_pair(Vector2d(-y, z), Vector2d(-y,z+1)));
    }
}


void
PaintbrushInteractionMode
::OnDraw()
{
  // Draw the outline of the paintbrush
  PaintbrushSettings pbs = 
    m_ParentUI->GetDriver()->GetGlobalState()->GetPaintbrushSettings();

  // Paint all the edges in the paintbrush definition
  const SNAPAppearanceSettings::Element &elt = 
    m_ParentUI->GetAppearanceSettings()->GetUIElement(
    SNAPAppearanceSettings::PAINTBRUSH_OUTLINE);

  // Build the mask edges
  BuildBrush(pbs);

  // Get the cursor position on the slice
  Vector3ui xCursorInteger = m_GlobalState->GetCrosshairsPosition();
  Vector3f xCursorImage = to_float(xCursorInteger) + Vector3f(0.5f);
  Vector3f xCursorSlice = m_Parent->MapImageToSlice(xCursorImage);

  // Set line properties
  glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);

  // Apply the line properties
  glColor3dv(elt.NormalColor.data_block());
  SNAPAppearanceSettings::ApplyUIElementLineSettings(elt);

  // Refit matrix so that the lines are centered on the current pixel
  glPushMatrix();
  glTranslated( xCursorSlice(0), xCursorSlice(1), 0.0 );

  // Start drawing the lines
  glBegin(GL_LINES);
  
  // Draw each of the edges. The edges are offset by the center of 
  // the current pixel
  for(EdgeList::iterator it = m_MaskEdges.begin(); it != m_MaskEdges.end(); ++it)
    {
    glVertex2d(it->first(0), it->first(1));
    glVertex2d(it->second(0), it->second(1));
    }

  glVertex2d(-1.0, 1.0);
  glVertex2d(1.0, -1.0);
  glVertex2d(-1.0, -1.0);
  glVertex2d(1.0, 1.0);

  // Finish drawing
  glEnd();

  // Pop the matrix
  glPopMatrix();

  // Pop the attributes
  glPopAttrib();
}

int
PaintbrushInteractionMode
::OnMousePress(FLTKEvent const &event)
{
  return 0;
}

int
PaintbrushInteractionMode
::OnMouseRelease(FLTKEvent const &event, FLTKEvent const &pressEvent)
{
  return 0;
}

int
PaintbrushInteractionMode
::OnMouseDrag(FLTKEvent const &event, FLTKEvent const &pressEvent)
{
  return 0;
}

int
PaintbrushInteractionMode
::OnKeyDown(FLTKEvent const &event)
{
  return 0;
}

int
PaintbrushInteractionMode
::OnShortcut(FLTKEvent const &event)
{
  return 0;
}


