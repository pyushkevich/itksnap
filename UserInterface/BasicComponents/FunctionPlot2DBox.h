/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: FunctionPlot2DBox.h,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:16 $
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
#ifndef __FunctionPlot2DBox_h_
#define __FunctionPlot2DBox_h_

#include "FunctionPlot2D.h"
#include "FLTKCanvas.h"

/**
 * \class FunctionPlot2DBox
 * \brief An FLTK Box used for drawing a plot using the above object
 */
class FunctionPlot2DBox : public FLTKCanvas
{
public:
  /** The standard FLTK window constructor */
  FunctionPlot2DBox(int x,int y,int w,int h,const char *label=NULL);
  
  /** Access the plotting object associated with this window */
  FunctionPlot2D &GetPlotter()
  {
    return m_Plotter;
  }

  /** The draw method */
  void draw();

private:
  FunctionPlot2D m_Plotter;
};

#endif
