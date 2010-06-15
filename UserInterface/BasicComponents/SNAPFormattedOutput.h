/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SNAPFormattedOutput.h,v $
  Language:  C++
  Date:      $Date: 2010/06/15 16:27:44 $
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
#ifndef __SNAPFormattedOutput_h_
#define __SNAPFormattedOutput_h_

#include "FL/Fl_Output.H"
#include <cstdio>
#include <cstring>

/**
 * \class SNAPFormattedOutput
 * \brief An extension of Fl_Output that can take double as input
 */
class SNAPFormattedOutput : public Fl_Output
{
public:
  SNAPFormattedOutput(int X, int Y, int W, int H,const char *l=0)
    : Fl_Output(X,Y,W,H,l) { set_format("%.4g"); }

  void set_format(const char *format)
    { strncpy(m_Format, format, 32); }

  const char *get_format() const
    { return m_Format; }

  void value(double val)
    { 
    sprintf(m_Buffer, m_Format, val);
    Fl_Output::value(m_Buffer);
    }

  void value(double val, const char *format)
    { set_format(format); value(val); }

protected:
  char m_Format[32], m_Buffer[128];
};


#endif
