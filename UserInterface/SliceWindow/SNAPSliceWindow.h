/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SNAPSliceWindow.h,v $
  Language:  C++
  Date:      $Date: 2009/08/25 19:46:18 $
  Version:   $Revision: 1.5 $
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

  // The interaction mode for bubble drawing
  BubblesInteractionMode *m_BubblesMode;
  
  // SNAP image data object displayed in this window (overrides parent's 
  // m_ImageData
  SNAPImageData *m_ImageData;

  // Preprocessed slice texture object
  OpenGLSliceTexture *m_SpeedTexture;

  // Segmentation slice texture object
  OpenGLSliceTexture *m_SnakeTexture;

  // The overlay texture object
  OpenGLSliceTexture *m_OverlayTexture;

  // Overlay and texture drawing are customized in this class
  void DrawOverlays();
  void DrawMainTexture();
  void DrawSegmentationTexture();  
};

#endif // SNAP Slice Window
