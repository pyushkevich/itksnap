/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: ColorMapBox.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:20 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __ColorMapBox_h_
#define __ColorMapBox_h_

#include "FLTKCanvas.h"
#include "SpeedColorMap.h"

/**
 * \class ColorMapBox
 * \brief An FLTK Box used to display a color map
 */
class ColorMapBox : public FLTKCanvas
{
public:
  /** The standard FLTK window constructor */
  ColorMapBox(int x,int y,int w,int h,const char *label=NULL);

  /** Set the current color map functor */
  void SetColorMap(const SpeedColorMap &map)
    { m_ColorMap = map; redraw(); }

  /** Set the range for the color map */
  void SetRange(double x0, double x1)
    { m_RangeStart = x0; m_RangeEnd = x1; }
  
  /** The draw method */
  void draw();

private:
  SpeedColorMap m_ColorMap;
  double m_RangeStart, m_RangeEnd;
};

#endif
