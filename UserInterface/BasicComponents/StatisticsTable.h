/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: StatisticsTable.h,v $
  Language:  C++
  Date:      $Date: 2010/05/31 19:52:37 $
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
#ifndef __StatisticsTable_h_
#define __StatisticsTable_h_

#include "FL/Fl_Table.H"
#include "FL/Fl_Table_Row.H"
#include "SNAPCommon.h"
#include "SegmentationStatistics.h"
#include "ColorLabelTable.h"

class StatisticsTable : public Fl_Table_Row
{
public:
  StatisticsTable(int x, int y, int w, int h, const char *l=0) 
    : Fl_Table_Row(x,y,w,h,l) { end(); }

  ~StatisticsTable() { }

  void SetSegmentationStatistics(
    const SegmentationStatistics &data, const ColorLabelTable &clt);

  std::string CopySelection();

protected:

  /* Draw table cell */
  void draw_cell(TableContext context,  		
    int R=0, int C=0, int X=0, int Y=0, int W=0, int H=0);

  std::vector< std::string > m_Header;
  std::vector< std::vector< std::string > > m_Body;

};

#endif

