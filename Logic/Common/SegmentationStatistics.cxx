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

using namespace std;

struct SegmentationStatisticsSource {
  string name;
  GreyImageWrapper *image;
  GreyImageWrapper::ConstIterator it; 
  GreyTypeToNativeFunctor funk;
};

void
SegmentationStatistics
::Compute(GenericImageData *id)
{
  // A list of image sources
  vector<SegmentationStatisticsSource> isrc;

  // Populate image sources
  if(id->IsGreyLoaded())
    {
    SegmentationStatisticsSource src;
    src.name = "image";
    src.image = id->GetGrey();
    src.it = src.image->GetImageConstIterator();
    src.funk = src.image->GetNativeMapping();
    isrc.push_back(src);
    }

  // Add all grey overlays
  size_t k = 1;
  GenericImageData::WrapperIterator it = id->GetOverlays()->begin();
  for(; it != id->GetOverlays()->end(); it++, k++)
    {
    // Is it a grey wrapper?
    GreyImageWrapper *wrapper = dynamic_cast<GreyImageWrapper *>(*it);
    if (wrapper)
      {
      SegmentationStatisticsSource src;
      ostringstream oss; oss << "ovl " << k;
      src.name = oss.str();
      src.image = wrapper;
      src.it = src.image->GetImageConstIterator();
      src.funk = src.image->GetNativeMapping();
      isrc.push_back(src);
      }
    }

  // Get the number of gray image layers
  size_t ngray = isrc.size();

  // Clear and initialize the statistics table
  for(size_t i = 0; i < MAX_COLOR_LABELS; i++)
    {
    m_Stats[i] = Entry();
    for(size_t j = 0; j < ngray; j++)
      {
      GrayStats gs;
      gs.layer_id = isrc[j].name;
      m_Stats[i].gray.push_back(gs);
      }
    }

  // Compute the statistics by iterating over each voxel
  for(LabelImageWrapper::ConstIterator itLabel = 
    id->GetSegmentation()->GetImageConstIterator();
    !itLabel.IsAtEnd(); ++itLabel)
    {
    // Get the label and the corresponding entry
    LabelType label = itLabel.Value();
    Entry &entry = m_Stats[label];

    // Increment number of voxels for this label
    entry.count++;

    // Update the gray statistics
    for(size_t j = 0; j < ngray; j++)
      {
      double v = isrc[j].funk( isrc[j].it.Value() );
      ++isrc[j].it;
      entry.gray[j].sum += v;
      entry.gray[j].sumsq += v * v;
      }
    }
  
  // Compute the size of a voxel, in mm^3
  const double *spacing = 
    id->GetMain()->GetImageBase()->GetSpacing().GetDataPointer();
  double volVoxel = spacing[0] * spacing[1] * spacing[2];
  
  // Compute the mean and standard deviation
  for (size_t i=0; i < MAX_COLOR_LABELS; i++)
    {
    Entry &entry = m_Stats[i];
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

  for (size_t i=1; i<MAX_COLOR_LABELS; i++) 
    {
    const ColorLabel &cl = clt.GetColorLabel(i);
    if(cl.IsValid() && m_Stats[i].count > 0)
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

