/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: BubblesInteractionMode.cxx,v $
  Language:  C++
  Date:      $Date: 2008/02/10 23:55:22 $
  Version:   $Revision: 1.3 $
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
#include "BubblesInteractionMode.h"

#include "IRISException.h"
#include "IRISApplication.h"
#include "IRISSliceWindow.h"
#include "UserInterfaceBase.h"
#include "IRISImageData.h"
#include <vnl/vnl_math.h>

#include <assert.h>
#include <cmath>

const GLubyte stipple[] = {
  0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
  0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
  0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
  0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
  0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
  0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
  0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
  0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
  0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
  0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
  0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
  0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
  0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
  0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
  0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
  0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55 };

BubblesInteractionMode
::BubblesInteractionMode(GenericSliceWindow *parent)
:GenericSliceWindow::EventHandler(parent)
{
}

void
BubblesInteractionMode
::OnDraw()
{
  // Draw the bubbles if necessary
  if (m_GlobalState->GetSnakeActive() == false)
    {
    // Get the list of bubbles
    std::vector<Bubble> bubbles = m_GlobalState->GetBubbleArray();

    // draw bubbles
    int numBubbles = bubbles.size();
    int activeBubble = m_GlobalState->GetActiveBubble();

    if (numBubbles > 0)
      {
      // Get the active color label 
      int currentcolor =  m_GlobalState->GetDrawingColorLabel();      
      ColorLabel cl = 
        m_Driver->GetColorLabelTable()->GetColorLabel(currentcolor);
      
      if (cl.IsValid())
        {
        // Get the current alpha blending factor for displaying overlays
        unsigned char alpha = m_GlobalState->GetSegmentationAlpha();
        
        // Get the color of the active color label
        unsigned char rgb[3];
        cl.GetRGBVector(rgb);

        // Get the current crosshairs position
        Vector3f cursorImage = 
          to_float(m_Driver->GetCursorPosition()) + Vector3f(0.5f);

        // Get the image space dimension that corresponds to this window
        int iid = m_Parent->m_ImageAxes[2];
        
        // Get the other essentials from the parent
        Vector3f scaling = m_Parent->m_SliceSpacing;       
        
        // Turn on alpha blending
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Create a filled circle object
        GLUquadricObj *object = gluNewQuadric();
        gluQuadricDrawStyle(object,GLU_FILL);

        // Draw each bubble
        for (int i = 0; i < numBubbles; i++)
          {  
          
          // Get the center and radius of the i-th bubble
          Vector3f ctrImage = to_float(bubbles[i].center) + Vector3f(0.5f);
          double radius = bubbles[i].radius;

          // Remap the center into slice coordinates
          Vector3f ctrSlice = m_Parent->MapImageToSlice(to_float(ctrImage));

          // Compute the offset from the center along the slice z-direction
          // in physical coordinates
          double dcenter = scaling(2) * (cursorImage(iid) - ctrImage(iid));
                    
          // Check if the bubble is intersected by the current slice plane
          if (dcenter >= radius || -dcenter >= radius) continue;
            
          // Compute the radius of the bubble in the cut plane
          double diskradius = sqrt(fabs(radius*radius - dcenter*dcenter));

          // Draw the bubble
          glColor4ub(rgb[0],rgb[1],rgb[2],alpha);
          glPushMatrix();

          glPushAttrib(GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT);
          if(activeBubble == i)
            {
            glEnable(GL_POLYGON_STIPPLE);
            glPolygonStipple(stipple);
            }
          
          glTranslatef(ctrSlice[0], ctrSlice[1], 0.0f);
          glScalef(1.0f / scaling(0),1.0f / scaling(1),1.0f);
          gluDisk(object,0,diskradius,100,1);
          glPopAttrib();

          // If the bubble is active, draw an outline around the bubble
          if(activeBubble == i)
            {
            glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);

            glEnable(GL_BLEND);
            glEnable(GL_LINE_SMOOTH);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glLineWidth(1.5);
            
            glColor4ub(
              255 - (255 - rgb[0]) / 2,
              255 - (255 - rgb[1]) / 2,
              255 - (255 - rgb[2]) / 2, 255);
            
            glBegin(GL_LINE_LOOP);
            for(unsigned int d = 0; d < 360; d+=2)
              {
              double rad = d * vnl_math::pi / 180.0;
              glVertex2f(diskradius * cos(rad), diskradius * sin(rad));
              }
            glEnd();
            glPopAttrib();
            }
          
          glPopMatrix();

          }

        gluDeleteQuadric(object);
        glDisable(GL_BLEND);
        }
      }
    }
}


