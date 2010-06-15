/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SNAPValueOutput.h,v $
  Language:  C++
  Date:      $Date: 2010/06/15 16:27:44 $
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
#ifndef __SNAPValueOutput_h_
#define __SNAPValueOutput_h_

#include "FL/Fl_Value_Output.H"
#include <cstdio>

/**
 * \class SNAPValueOutput
 * \brief An extension of Fl_Value_Output with specified number of significant
 * digits (uses %.*g format)
 */
class SNAPValueOutput : public Fl_Value_Output
{
public:
  SNAPValueOutput(int X, int Y, int W, int H,const char *l=0)
    : Fl_Value_Output(X,Y,W,H,l) { this->step(4); }

protected:

  int format(char *buffer)
    {
    double s = step();
    if(s >= 1.0)
      return sprintf(buffer, "%.*g", (int) s, this->value());
    else 
      return Fl_Value_Output::format(buffer);
    }

  int handle(int event)
    { return 0; }
};


#endif
