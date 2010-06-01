/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: MetaDataTable.cxx,v $
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
#include "MetaDataTable.h"
#include "FL/fl_draw.H"
#include "itkImage.h"
#include "itkMetaDataDictionary.h"
#include "itkMetaDataObject.h"
using namespace std;
using namespace itk;

string
get_rai_code(itk::SpatialOrientation::ValidCoordinateOrientationFlags code)
  {
  std::map<itk::SpatialOrientation::ValidCoordinateOrientationFlags, string> m_CodeToString;
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RIP] = "RIP";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LIP] = "LIP";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RSP] = "RSP";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LSP] = "LSP";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RIA] = "RIA";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LIA] = "LIA";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RSA] = "RSA";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LSA] = "LSA";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IRP] = "IRP";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ILP] = "ILP";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SRP] = "SRP";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SLP] = "SLP";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IRA] = "IRA";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ILA] = "ILA";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SRA] = "SRA";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SLA] = "SLA";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RPI] = "RPI";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LPI] = "LPI";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RAI] = "RAI";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LAI] = "LAI";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RPS] = "RPS";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LPS] = "LPS";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RAS] = "RAS";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LAS] = "LAS";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PRI] = "PRI";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PLI] = "PLI";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ARI] = "ARI";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ALI] = "ALI";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PRS] = "PRS";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PLS] = "PLS";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ARS] = "ARS";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ALS] = "ALS";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IPR] = "IPR";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SPR] = "SPR";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IAR] = "IAR";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SAR] = "SAR";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IPL] = "IPL";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SPL] = "SPL";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IAL] = "IAL";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SAL] = "SAL";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PIR] = "PIR";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PSR] = "PSR";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_AIR] = "AIR";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ASR] = "ASR";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PIL] = "PIL";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PSL] = "PSL";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_AIL] = "AIL";
  m_CodeToString[itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ASL] = "ASL";
  return m_CodeToString[code];
  }

template<class AnyType>
bool
try_get_metadata(itk::MetaDataDictionary &mdd, 
                   string &key, string &output, AnyType deflt)
  {
  AnyType v = deflt;
  if(itk::ExposeMetaData<AnyType>(mdd, key, v))
    {
    ostringstream oss;
    oss << v << endl;
    output = oss.str();
    return true;
    }
  else return false;
  }

void
MetaDataTable
::SetInputImage(itk::ImageBase<3> *image)
{
  static char buffer[256];

  cout << "SetInputImage " << endl;

  // Get the data
  m_Header.clear();
  m_Body.clear();

  m_Header.push_back("Key");
  m_Header.push_back("Value");

  // Get the dictionary
  itk::MetaDataDictionary &mdd = image->GetMetaDataDictionary();
  itk::MetaDataDictionary::ConstIterator itMeta;
  for(itMeta = mdd.Begin(); itMeta != mdd.End(); ++itMeta)
    {
    // Get the metadata as a generic object
    string key = itMeta->first, v_string;
    cout << key << endl;
    string value;

    // Orientation flag object
    itk::SpatialOrientation::ValidCoordinateOrientationFlags v_oflags = 
      itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_INVALID;

    // Is the value a string?
    if(itk::ExposeMetaData<string>(mdd, key, value))
      {
      // For some weird reason, some of the strings returned by this method
      // contain '\0' characters. We will replace them by spaces
      ostringstream sout("");
      for(unsigned int i=0;i<value.length();i++)
        if(value[i] >= ' ') sout << value[i];
      value = sout.str();

      // Make sure the value has more than blanks
      if(value.find_first_not_of(" ") == v_string.npos)
        value="";
      }
    else if(itk::ExposeMetaData(mdd, key, v_oflags))
      {
      value = get_rai_code(v_oflags);
      }
    else 
      {
      bool rc = false;
      if(!rc) rc |= try_get_metadata<double>(mdd, key, value, 0);
      if(!rc) rc |= try_get_metadata<float>(mdd, key, value, 0);
      if(!rc) rc |= try_get_metadata<int>(mdd, key, value, 0);
      if(!rc) rc |= try_get_metadata<unsigned int>(mdd, key, value, 0);
      if(!rc) rc |= try_get_metadata<long>(mdd, key, value, 0);
      if(!rc) rc |= try_get_metadata<unsigned long>(mdd, key, value, 0);
      if(!rc) rc |= try_get_metadata<short>(mdd, key, value, 0);
      if(!rc) rc |= try_get_metadata<unsigned short>(mdd, key, value, 0);
      if(!rc) rc |= try_get_metadata<char>(mdd, key, value, 0);
      if(!rc) rc |= try_get_metadata<unsigned char>(mdd, key, value, 0);

      if(!rc)
        {
        ostringstream oss;
        oss << "object of type " 
          << itMeta->second->GetMetaDataObjectTypeName();
        value = oss.str();
        }
      }

    // Store the values
    vector<string> row;
    row.push_back(key);
    row.push_back(value);
    m_Body.push_back(row);
    }

  // Set the number of rows and columns
  this->rows(m_Body.size());
  this->cols(m_Header.size()-1);
  this->row_header(1);
  this->col_header(1);
  this->col_resize(4);

  // Set the column widths (allow for scrollbar)
  this->col_width_all(140);
  this->row_header_width(140);
}


void
MetaDataTable
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
      fl_draw(text.c_str(), X+3, Y, W-6, H, FL_ALIGN_LEFT);
      }
    fl_pop_clip();
    break;

  default:
    break;
  }
}
