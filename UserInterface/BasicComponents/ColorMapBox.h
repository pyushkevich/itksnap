/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ColorMapBox.h,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:15 $
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
