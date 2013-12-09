/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SegmentationStatistics.cxx,v $
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
#include "SegmentationStatistics.h"
#include "GenericImageData.h"

#include <iostream>
#include <iomanip>

using namespace std;



void
SegmentationStatistics
::Compute(GenericImageData *id)
{
  // TODO: improve efficiency by using filters to integrate label intensities

  // A list of image sources
  vector<ScalarImageWrapperBase *> layers;

  // Clear the list of column names
  m_ImageStatisticsColumnNames.clear();

  // Find all the images available for statistics computation
  for(LayerIterator it(id, MAIN_ROLE |
                           OVERLAY_ROLE);
      !it.IsAtEnd(); ++it)
    {
    ScalarImageWrapperBase *lscalar = it.GetLayerAsScalar();
    if(lscalar)
      {
      m_ImageStatisticsColumnNames.push_back(lscalar->GetNickname());
      layers.push_back(lscalar);
      }
    else
      {
      VectorImageWrapperBase *lvector = it.GetLayerAsVector();
      for(int j = 0; j < lvector->GetNumberOfComponents(); j++)
        {
        std::ostringstream oss;
        oss << lvector->GetNickname();
        if(lvector->GetNumberOfComponents() > 1)
          oss << " [" << j << "]";
        m_ImageStatisticsColumnNames.push_back(oss.str());
        layers.push_back(lvector->GetScalarRepresentation(
              SCALAR_REP_COMPONENT, j));
        }
      }
    }

  // Get the number of gray image layers
  size_t ngray = layers.size();

  // Clear and initialize the statistics table
  m_Stats.clear();

  // Aggregate the statistical data
  for(LabelImageWrapper::ConstIterator itLabel =
    id->GetSegmentation()->GetImageConstIterator();
    !itLabel.IsAtEnd(); ++itLabel)
    {
    // Get the label and the corresponding entry
    LabelType label = itLabel.Value();
    Entry &entry = m_Stats[label];
    if(entry.count == 0)
      {
      entry.gray.resize(ngray);
      }

    // Increase the count
    entry.count++;

    // Integrate the image data
    itk::Index<3> idx = itLabel.GetIndex();
    for(size_t j = 0; j < ngray; j++)
      {
      double v = layers[j]->GetVoxelMappedToNative(idx);
      entry.gray[j].sum += v;
      entry.gray[j].sumsq += v * v;
      }
    }

  // Compute the size of a voxel, in mm^3
  const double *spacing = 
    id->GetMain()->GetImageBase()->GetSpacing().GetDataPointer();
  double volVoxel = spacing[0] * spacing[1] * spacing[2];
  
  // Compute the mean and standard deviation
  for(EntryMap::iterator it = m_Stats.begin(); it != m_Stats.end(); ++it)
    {
    Entry &entry = it->second;
    for(size_t j = 0; j < ngray; j++)
      {
      entry.gray[j].mean = entry.gray[j].sum / entry.count;
      entry.gray[j].sd = sqrt(
        (entry.gray[j].sumsq - entry.gray[j].sum * entry.gray[j].mean) 
        / (entry.count - 1));
      }
    entry.volume_mm3 = entry.count * volVoxel;
    }
}

void 
SegmentationStatistics
::ExportLegacy(ostream &fout, const ColorLabelTable &clt)
{
  // Write voxel volumes to the file
  fout << "##########################################################" << std::endl;
  fout << "# SNAP Voxel Count File" << std::endl;
  fout << "# File format:" << std::endl;
  fout << "# LABEL: ID / NUMBER / VOLUME / MEAN / SD" << std::endl;
  fout << "# Fields:" << std::endl;
  fout << "#    LABEL         Label description" << std::endl;
  fout << "#    ID            The numerical id of the label" << std::endl;
  fout << "#    NUMBER        Number of voxels that have that label " << std::endl;
  fout << "#    VOLUME        Volume of those voxels in cubic mm " << std::endl;
  fout << "#    MEAN          Mean intensity of those voxels " << std::endl;
  fout << "#    SD            Standard deviation of those voxels " << std::endl;
  fout << "##########################################################" << std::endl;

  for (ColorLabelTable::ValidLabelConstIterator it = clt.begin();
       it != clt.end(); it++)
    {
    LabelType i = it->first;
    ColorLabel cl = clt.GetColorLabel(i);
    if(m_Stats[i].count > 0)
      {
      fout << std::left << std::setw(40) << cl.GetLabel() << ": ";
      fout << std::right << std::setw(4) << i << " / ";
      fout << std::right << std::setw(10) << m_Stats[i].count << " / ";
      fout << std::setw(10) << (m_Stats[i].volume_mm3);
     
      for(size_t j = 0; j < m_Stats[i].gray.size(); j++)
        {
        fout << " / " << std::internal << std::setw(10) 
          << m_Stats[i].gray[j].mean << " / " << std::setw(10) 
          << m_Stats[i].gray[j].sd;
        }

      fout << endl;
      }      
    }
}

#include "itksys/SystemTools.hxx"

void SegmentationStatistics
::Export(ostream &oss, const string &colsep, const ColorLabelTable &clt)
{
  // Write out the header
  oss << "Label Id" << colsep;
  oss << "Label Name" << colsep;
  oss << "Number Of Voxels" << colsep;
  oss << "Volume (mm^3)";

  // Print the list of column names
  for(int i = 0; i < m_ImageStatisticsColumnNames.size(); i++)
    {
    std::string colname = m_ImageStatisticsColumnNames[i];
    itksys::SystemTools::ReplaceString(colname, colsep.c_str(), " ");

    oss << colsep << "Image mean (" << colname << ")";
    oss << colsep << "Image stdev (" << colname << ")";
    }

  // Endline
  oss << std::endl;

  // Write each row
  for(EntryMap::iterator it = m_Stats.begin(); it != m_Stats.end(); ++it)
    {
    LabelType i = it->first;
    Entry &entry = it->second;
    oss << i << colsep;

    std::string label(clt.GetColorLabel(i).GetLabel());
    itksys::SystemTools::ReplaceString(label, colsep.c_str(), " ");
    oss << label << colsep;

    oss << entry.count << colsep;
    oss << entry.volume_mm3;

    for(int j = 0; j < entry.gray.size(); j++)
      {
      oss << colsep << entry.gray[j].mean;
      oss << colsep << entry.gray[j].sd;
      }

    oss << std::endl;
    }
}

