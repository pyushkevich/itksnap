/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: StatisticsTable.cxx,v $
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
#include "StatisticsTable.h"
#include "FL/fl_draw.H"
using namespace std;

void
StatisticsTable
::SetSegmentationStatistics(
  const SegmentationStatistics &data, const ColorLabelTable &clt)
{
  static char buffer[256];

  // Get the data
  m_Header.clear();
  m_Body.clear();

  // Get first entry
  SegmentationStatistics::Entry e = data.GetStats()[0];

  // Set the columns
  m_Header.push_back("Label");
  m_Header.push_back("Label Id");
  m_Header.push_back("# Voxels");
  m_Header.push_back("Volume (mm^3)");
  for(size_t i = 0; i < e.gray.size(); i++)
    {
    sprintf(buffer, "Mean(%s)", e.gray[i].layer_id.c_str());
    m_Header.push_back(buffer);

    sprintf(buffer, "S.D.(%s)", e.gray[i].layer_id.c_str());
    m_Header.push_back(buffer);
    }

  // Set the body of the table
  for(size_t i = 0; i < MAX_COLOR_LABELS; i++)
    {
    SegmentationStatistics::Entry e = data.GetStats()[i];
    const ColorLabel &cl = clt.GetColorLabel(i);
    if(e.count == 0) 
      continue;

    vector<string> m_Row;
    m_Row.push_back(cl.GetLabel());
    
    sprintf(buffer, "%d", (int) i);
    m_Row.push_back(buffer);

    sprintf(buffer, "%d", (int) e.count);
    m_Row.push_back(buffer);
    
    sprintf(buffer, "%g", e.volume_mm3);
    m_Row.push_back(buffer);
    
    for(size_t i = 0; i < e.gray.size(); i++)
      {
      sprintf(buffer, "%g", e.gray[i].mean);
      m_Row.push_back(buffer);

      sprintf(buffer, "%g", e.gray[i].sd);
      m_Row.push_back(buffer);
      }

    m_Body.push_back(m_Row);
    }

  // Set the number of rows and columns
  this->rows(m_Body.size());
  this->cols(m_Header.size()-1);
  this->row_header(1);
  this->col_header(1);
  this->col_resize(4);

  // Set the column widths (allow for scrollbar)
  this->col_width_all(100);
  if(m_Body.size() > 8)
    this->row_header_width(144);
  else
    this->row_header_width(160);
}

std::string
StatisticsTable
::CopySelection()
{
  ostringstream oss;

  // Count the number of selected rows
  size_t nsel = 0;
  for(size_t r = 0; r < m_Body.size(); r++)
    if(this->row_selected(r))
      nsel++;

  // If no selection, include the header too
  if(nsel == 0)
    {
    for(size_t j = 0; j < m_Header.size(); j++)
      {
      if(j > 0)
        oss << "\t";
      oss << m_Header[j];
      }
    oss << endl;
    }

  // Go through the selected rows
  for(size_t r = 0; r < m_Body.size(); r++)
    {
    if(this->row_selected(r) || nsel == 0)
      {
      for(size_t j = 0; j < m_Body[r].size(); j++)
        {
        if(j > 0)
          oss << "\t";
        oss << m_Body[r][j];
        }
      oss << endl;
      }
    }

  return oss.str();
}


void
StatisticsTable
::draw_cell(TableContext context,  		
    int R, int C, int X, int Y, int W, int H)
{
  string text;


  // If data is not initialized, return
  if(m_Header.size() == 0)
    return;

  // Actual drawing
  switch(context)
    {
  case CONTEXT_STARTPAGE:
    fl_font(FL_COURIER, 12);
    return;

  case CONTEXT_ROW_HEADER:
    text = m_Body[R][0];
    fl_push_clip(X, Y, W, H);
    fl_draw_box(FL_THIN_UP_BOX, X, Y, W, H, FL_LIGHT1);
    fl_pop_clip();

    fl_push_clip(X+3, Y, W-6, H);
    fl_color(FL_BLACK);
    fl_draw(text.c_str(), X+3, Y, W-6, H, FL_ALIGN_LEFT);
    fl_pop_clip();
    break;

  case CONTEXT_COL_HEADER:
    text = m_Header[C+1];

    fl_push_clip(X, Y, W, H);
      {
      fl_draw_box(FL_THIN_UP_BOX, X, Y, W, H, FL_LIGHT1);
      fl_color(FL_BLACK);
      fl_draw(text.c_str(), X, Y, W, H, FL_ALIGN_CENTER);
      }
    fl_pop_clip();
    break;

  case CONTEXT_CELL:
    text = m_Body[R][C+1];

    fl_push_clip(X, Y, W, H);
    fl_draw_box(FL_THIN_UP_BOX, X, Y, W, H, 
      row_selected(R) ? selection_color() : FL_WHITE);
    fl_pop_clip();

    fl_push_clip(X+3, Y, W-6, H);
      {
      // TEXT
      fl_color(FL_BLACK);
      fl_draw(text.c_str(), X+3, Y, W-6, H, FL_ALIGN_RIGHT);
      }
    fl_pop_clip();
    break;

  default:
    break;
  }
}
