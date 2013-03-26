/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: OpenGLSliceTexture.h,v $
  Language:  C++
  Date:      $Date: 2009/08/25 19:44:25 $
  Version:   $Revision: 1.9 $
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
#ifndef __OpenGLSliceTexture_h_
#define __OpenGLSliceTexture_h_

#include "SNAPCommon.h"
#include "SNAPOpenGL.h"

#ifndef _WIN32
#ifndef GLU_VERSION_1_2
#define GLU_VERSION_1_2 1
#endif
#endif

#include "itkImage.h"

/**
 * \class OpenGLSliceTexture
 * \brief This class is used to turn a 2D ITK image of (arbitrary) type
 * into a GL texture.  
 *
 * The calls to Update will make sure that the texture is up to date.  
 */
template <class TPixel>
class OpenGLSliceTexture 
{
public:
  // Image typedefs
  typedef itk::Image<TPixel, 2> ImageType;
  typedef typename itk::SmartPointer<ImageType> ImagePointer;

  /** Constructor, initializes the texture object */
  OpenGLSliceTexture();
  OpenGLSliceTexture(GLuint, GLenum);

  /** Destructor, deallocates texture memory */
  virtual ~OpenGLSliceTexture();
  
  irisGetMacro(Image, const ImageType *);

  /** Pass in a pointer to a 2D image */
  void SetImage(ImageType *inImage);

  /** Get the dimensions of the texture image, which are powers of 2 */
  irisGetMacro(TextureSize,Vector2ui);

  /** Get the GL texture number automatically allocated by this object */
  irisGetMacro(TextureIndex,int);

  /** Set the number of components used in call to glTextureImage */
  irisSetMacro(GlComponents,GLuint);

  /** Get the format (e.g. GL_LUMINANCE) in call to glTextureImage */
  irisSetMacro(GlFormat,GLenum);

  /** Get the type (e.g. GL_UNSIGNED_INT) in call to glTextureImage */
  irisSetMacro(GlType,GLenum);

  /**
   * Make sure that the texture is up to date (reflects the image)
   */
  void Update();

  /**
   * Set the interpolation mode for the texture. If the interpolation mode
   * is changed, Update() will be called on the next Draw() command. The value
   * must be GL_NEAREST or GL_LINEAR
   */
  void SetInterpolation(GLenum newmode);

  /**
   * Draw the texture in the current OpenGL context on a polygon with vertices
   * (0,0) - (size_x,size_y). Paramters are the background color of the polygon
   */
  void Draw(const Vector3d &clrBackground);

  /**
   * Draw the texture in transparent mode, with given level of alpha blending.
   */
  void DrawTransparent(double alpha);

private:
  
  // The dimensions of the texture as stored in memory
  Vector2ui m_TextureSize;

  // The pointer to the image from which the texture is computed
  ImagePointer m_Image;

  // The texture number (index)
  GLuint m_TextureIndex;

  // Has the texture been initialized?
  bool m_IsTextureInitalized;

  // The pipeline time of the source image (vs. our pipeline time)
  unsigned long m_UpdateTime;

  // The number of components for Gl op
  GLuint m_GlComponents;

  // The format for Gl op
  GLenum m_GlFormat;

  // The type for Gl op
  GLenum m_GlType;

  // Interpolation mode
  GLenum m_InterpolationMode;
};

#endif // __OpenGLSliceTexture_h_
