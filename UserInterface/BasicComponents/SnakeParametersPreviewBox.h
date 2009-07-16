/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SnakeParametersPreviewBox.h,v $
  Language:  C++
  Date:      $Date: 2009/07/16 22:02:27 $
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

#ifndef __SnakeParametersPreviewBox_h_
#define __SnakeParametersPreviewBox_h_

#include "FLTKCanvas.h"
#include "SnakeParameters.h"
#include "itkRGBAPixel.h"

class OpenGLSliceTexture;
class SnakeParametersPreviewPipeline;
class SnakeParametersUILogic;

/**
 * \class SnakeParametersPreviewBox
 * \brief A user interface component used to preview snake 
 * parameters in 2D.  
 * 
 * This component displays a portion of an image and a curve representing
 * an evolving snake.  Using the snake equation parameters that the user specifies,
 * forces normal to the boundary of the image are shown as vectors
 */
class SnakeParametersPreviewBox : public FLTKCanvas
{
public:
  SnakeParametersPreviewBox(int x, int y, int w, int h, const char *label=0);
  virtual ~SnakeParametersPreviewBox();

  /** A preview pipeline that has the logic of this class */
  irisSetMacro(Pipeline,SnakeParametersPreviewPipeline *);
  irisGetMacro(Pipeline,SnakeParametersPreviewPipeline *);

  irisSetMacro(ParentUI, SnakeParametersUILogic *);
  irisGetMacro(ParentUI, SnakeParametersUILogic *);
  
  /** An enumeration of different display modes for this widget */
  enum DisplayMode 
    {
    CURVATURE_FORCE=0,ADVECTION_FORCE,PROPAGATION_FORCE,TOTAL_FORCE
    };

  /** Set the display mode */
  irisSetMacro(ForceToDisplay,DisplayMode);
  
  /** Draw method - paints the widget */
  void draw();
  
protected:
  
  // Texture type for drawing speed images
  typedef itk::RGBAPixel<unsigned char> RGBAType;

  /** Parent UI */
  SnakeParametersUILogic *m_ParentUI;

  /** Preview pipeline logic */
  SnakeParametersPreviewPipeline *m_Pipeline;
  
  /** A texture object used to store the image */
  OpenGLSliceTexture *m_Texture;
  
  /** Which force is being displayed? */
  DisplayMode m_ForceToDisplay;

  /** An interaction mode for point manipulation */
  class Interactor : public InteractionMode {
  public:
    Interactor(SnakeParametersPreviewBox *owner);

    int OnMousePress(const FLTKEvent &event);
    int OnMouseRelease(const FLTKEvent &event, const FLTKEvent &irisNotUsed(pressEvent));
    int OnMouseDrag(const FLTKEvent &event, const FLTKEvent &irisNotUsed(pressEvent));
    void OnDraw();
  private:
    SnakeParametersPreviewBox *m_Owner;
    bool m_ControlsVisible;
    bool m_ControlPicked;
    unsigned int m_ActiveControl;
  };

  // Allow private access to the interactor
  friend class SnakeParametersPreviewBox::Interactor;

  /** The interactor */
  Interactor m_Interactor;
};





#endif // __SnakeParametersPreviewBox_h_
