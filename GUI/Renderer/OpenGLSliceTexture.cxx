/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: OpenGLSliceTexture.cxx,v $
  Language:  C++
  Date:      $Date: 2009/08/25 19:44:25 $
  Version:   $Revision: 1.2 $
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
#include "OpenGLSliceTexture.h"
#include "ImageWrapper.h"


template<class TPixel>
OpenGLSliceTexture<TPixel>
::OpenGLSliceTexture()
{
  // Set to -1 to force a call to 'generate'
  m_IsTextureInitalized = false;

  // Set the update time to -1
  m_UpdateTime = 0;

  // Init the GL settings to uchar, luminance defautls, which are harmless
  m_GlComponents = 1;
  m_GlFormat = GL_LUMINANCE;
  m_GlType = GL_UNSIGNED_BYTE;
  m_InterpolationMode = GL_NEAREST;
}

template<class TPixel>
OpenGLSliceTexture<TPixel>
::OpenGLSliceTexture(GLuint components, GLenum format)
{
  // Set to -1 to force a call to 'generate'
  m_IsTextureInitalized = false;

  // Set the update time to -1
  m_UpdateTime = 0;

  // Init the GL settings to uchar, luminance defautls, which are harmless
  m_GlComponents = components;
  m_GlFormat = format;
  m_GlType = GL_UNSIGNED_BYTE;
  m_InterpolationMode = GL_NEAREST;
}

template<class TPixel>
OpenGLSliceTexture<TPixel>
::~OpenGLSliceTexture()
{
  if(m_IsTextureInitalized)
    glDeleteTextures(1,&m_TextureIndex);
}

template<class TPixel>
void
OpenGLSliceTexture<TPixel>
::SetInterpolation(GLenum interp)
{
  assert(interp == GL_LINEAR || interp == GL_NEAREST);
  if(interp != m_InterpolationMode)
    {
    m_InterpolationMode = interp;
    m_UpdateTime = 0; // make it out-of-date
    }
}


template<class TPixel>
void
OpenGLSliceTexture<TPixel>
::Update()
{
  // Better have an image
  assert(m_Image);

  // Update the image (necessary?)
  if(m_Image->GetSource())
    m_Image->GetSource()->UpdateLargestPossibleRegion();

  // Check if everything is up-to-date and no computation is needed
  if (m_IsTextureInitalized && m_UpdateTime == m_Image->GetPipelineMTime())
    {
    return;
    }

  // Promote the image dimensions to powers of 2
  itk::Size<2> szImage = m_Image->GetLargestPossibleRegion().GetSize();
  m_TextureSize = Vector2ui(1);

  // Use shift to quickly double the coordinates
  for (unsigned int i=0;i<2;i++)
    while (m_TextureSize(i) < szImage[i])
      m_TextureSize(i) <<= 1;

  // Create the texture index if necessary
  if(!m_IsTextureInitalized)
    {
    // Generate one texture
    glGenTextures(1,&m_TextureIndex);
    m_IsTextureInitalized = true;
    }

  // Select the texture for pixel pumping
  glBindTexture(GL_TEXTURE_2D,m_TextureIndex);

  // Properties for the texture
  glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_InterpolationMode );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_InterpolationMode );

  // Turn off modulo-4 rounding in GL
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);

  // Allocate texture of slightly bigger size
  glTexImage2D(GL_TEXTURE_2D, 0, m_GlComponents,
    m_TextureSize(0), m_TextureSize(1),
    0, m_GlFormat, m_GlType, NULL);

  // Copy a subtexture of correct size into the image
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, szImage[0], szImage[1],
                  m_GlFormat, m_GlType, m_Image->GetBufferPointer());

  // Remember the image's timestamp
  m_UpdateTime = m_Image->GetPipelineMTime();
}


template<class TPixel>
void
OpenGLSliceTexture<TPixel>
::Draw(const Vector3d &clrBackground)
{
  // Update the texture
  Update();

  // Should have a texture number
  assert(m_IsTextureInitalized);

  // GL settings
  glPushAttrib(GL_TEXTURE_BIT);
  glEnable(GL_TEXTURE_2D);

  // Select our texture
  glBindTexture(GL_TEXTURE_2D,m_TextureIndex);

  // Set the color to the background color
  glColor3dv(clrBackground.data_block());

  int w = m_Image->GetBufferedRegion().GetSize()[0];
  int h = m_Image->GetBufferedRegion().GetSize()[1];
  double tx = w * 1.0 / m_TextureSize(0);
  double ty = h * 1.0 / m_TextureSize(1);

  // Draw quad 
  glBegin(GL_QUADS);
  glTexCoord2d(0,0);
  glVertex2d(0,0);
  glTexCoord2d(0,ty);
  glVertex2d(0,h);
  glTexCoord2d(tx,ty);
  glVertex2d(w,h);
  glTexCoord2d(tx,0);
  glVertex2d(w,0);
  glEnd();

  glPopAttrib();
}


template<class TPixel>
void
OpenGLSliceTexture<TPixel>
::DrawTransparent(double alpha)
{
  // Update the texture
  Update();

  // Should have a texture number
  assert(m_IsTextureInitalized);

  // GL settings
  glPushAttrib(GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT);
  glEnable(GL_TEXTURE_2D);

  // Turn on alpha blending
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

  // Select our texture
  glBindTexture(GL_TEXTURE_2D,m_TextureIndex);

  // Set the color to white
  glColor4ub(255,255,255,(unsigned char)(alpha * 255));
    
  int w = m_Image->GetBufferedRegion().GetSize()[0];
  int h = m_Image->GetBufferedRegion().GetSize()[1];
  double tx = w * 1.0 / m_TextureSize(0);
  double ty = h * 1.0 / m_TextureSize(1);

  // Draw quad 
  glBegin(GL_QUADS);
  glTexCoord2d(0,0);
  glVertex2d(0,0);
  glTexCoord2d(0,ty);
  glVertex2d(0,h);
  glTexCoord2d(tx,ty);
  glVertex2d(w,h);
  glTexCoord2d(tx,0);
  glVertex2d(w,0);
  glEnd();

  glPopAttrib();
}

template<class TPixel>
void OpenGLSliceTexture<TPixel>::SetImage(ImageType *inImage)
{
  m_Image = inImage;
  m_Image->GetSource()->UpdateLargestPossibleRegion();
  m_UpdateTime = 0;
}

// Explicit instantiation of templates
template class OpenGLSliceTexture<ImageWrapperBase::DisplayPixelType>;
