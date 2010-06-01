/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: MetaDataTable.h,v $
  Language:  C++
  Date:      $Date: 2010/06/01 07:27:30 $
  Version:   $Revision: 1.1 $
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
#ifndef __MetaDataTable_h_
#define __MetaDataTable_h_

#include "FL/Fl_Table.H"
#include "FL/Fl_Table_Row.H"
#include "SNAPCommon.h"
#include <vector>
#include <string>

namespace itk { template <unsigned int VDim> class ImageBase; }

class MetaDataTable : public Fl_Table_Row
{
public:
  MetaDataTable(int x, int y, int w, int h, const char *l=0) 
    : Fl_Table_Row(x,y,w,h,l) { end(); }

  ~MetaDataTable() { }

  void SetInputImage(itk::ImageBase<3> *image);

protected:

  /* Draw table cell */
  void draw_cell(TableContext context,  		
    int R=0, int C=0, int X=0, int Y=0, int W=0, int H=0);

  std::vector< std::string > m_Header;
  std::vector< std::vector< std::string > > m_Body;

};

#endif

