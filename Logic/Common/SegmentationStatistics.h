/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SegmentationStatistics.h,v $
  Language:  C++
  Date:      $Date: 2010/06/15 16:54:35 $
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
#ifndef __SegmentationStatistics_h_
#define __SegmentationStatistics_h_

#include "SNAPCommon.h"
#include <vector>
#include <string>
#include <iostream>
#include <map>

class GenericImageData;
class ColorLabelTable;
class ScalarImageWrapperBase;

namespace itk {
  template <unsigned int VDim> class ImageRegion;
  template <unsigned int VDim> class Index;
}

class SegmentationStatistics
{
public:

  /* Data structure cooresponding to a gray overlay image */
  struct GrayStats { 
    std::string layer_id;
    double sum, sumsq, mean, sd;
    GrayStats() : sum(0), sumsq(0), mean(0), sd() {} 
  };

  /* Data structure corresponding to a row in the statistics table */
  struct Entry {
    unsigned long int count;
    double volume_mm3;
    vnl_vector<double> sum, sumsq, mean, stdev;
    Entry() : count(0),volume_mm3(0) {}
    void resize(int n) {
      sum.set_size(n); sum.fill(0);
      sumsq.set_size(n); sumsq.fill(0);
      mean.set_size(n); mean.fill(0);
      stdev.set_size(n); stdev.fill(0);
    }
  };

  typedef std::map<LabelType, Entry> EntryMap;

  /* Compute statistics from a segmentation image */
  void Compute(GenericImageData *id);
  
  /* Export to a text file using legacy format */
  void ExportLegacy(std::ostream &oss, const ColorLabelTable &clt);

  /* Export to a CSV or text file */
  void Export(std::ostream &oss, const std::string &colsep, const ColorLabelTable &clt);

  const EntryMap &GetStats() const
    { return m_Stats; }

  const std::vector<std::string> &GetImageStatisticsColumns() const
    { return m_ImageStatisticsColumnNames; }

private:

  // Label statistics
  EntryMap m_Stats;

  // Column information
  std::vector<std::string> m_ImageStatisticsColumnNames;
  
  void RecordRunLength(
      size_t ngray,
      std::vector<ScalarImageWrapperBase *> &layers,
      itk::ImageRegion<3> &region,
      itk::Index<3> &runStart,
      long runLength,
      Entry *cachedEntry);
};

#endif
