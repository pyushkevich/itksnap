/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SNAPSliceWindow.h,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:28 $
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
#ifndef __SNAPSliceWindow_h_
#define __SNAPSliceWindow_h_

#include "GenericSliceWindow.h"

// Forward references
class SNAPImageData;
class BubblesInteractionMode;
 
/**
 * \class SNAPSliceWindow
 * \brief The window used to display slices in SnAP part of the application.  
 *
 * SnAP not only has the original greyscale image, but also a floating 
 * point preprocessing image.  The overlays for SnAP include bubble display.
 */
class SNAPSliceWindow : public GenericSliceWindow
{
public:

  SNAPSliceWindow(int id,UserInterfaceBase *parentUI, FLTKCanvas *canvas);
  ~SNAPSliceWindow();

  /** Overrides the parent's method */
  void InitializeSlice(GenericImageData *imageData);

protected:

  // Texture object type associated with the preprocessing (speed) image
  typedef OpenGLSliceTexture<RGBAType> SpeedTextureType;

  // Texture object type associated with the active segmentation image
  typedef OpenGLSliceTexture<RGBAType> SnakeTextureType;
  
  // Another texture object type used to display thresholding overlays, etc
  typedef OpenGLSliceTexture<RGBAType> OverlayTextureType;

  // The interaction mode for bubble drawing
  BubblesInteractionMode *m_BubblesMode;
  
  // SNAP image data object displayed in this window (overrides parent's 
  // m_ImageData
  SNAPImageData *m_ImageData;

  // Preprocessed slice texture object
  SpeedTextureType *m_SpeedTexture;

  // Segmentation slice texture object
  SnakeTextureType *m_SnakeTexture;

  // The overlay texture object
  OverlayTextureType *m_OverlayTexture;

  // Overlay and texture drawing are customized in this class
  void DrawOverlays();
  void DrawGreyTexture();
  void DrawSegmentationTexture();  
};

#endif // SNAP Slice Window
