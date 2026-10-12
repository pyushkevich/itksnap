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
#include "IRISApplication.h"
#include "ImageCollectionConstIteratorWithIndex.h"
#include "ImageLabelIntegrator.h"
#include "RLEImageRegionIterator.h"

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>


using namespace std;


// TODO: improve efficiency by using filters to integrate label intensities
void
SegmentationStatistics
::Compute(IRISApplication *app)
{
  // Get the current image data
  GenericImageData *id = app->GetCurrentImageData();

  // Get the selected segmentation layer
  LabelImageWrapper *seg = app->GetSelectedSegmentationLayer();

  // Runs of voxels are taken from this segmentation, so their indices are in
  // its own grid. Everything below uses that grid, so the statistics do not
  // depend on which layer currently defines the reference space.
  auto *seg_space = seg->GetImageBase();

  // A list of image sources
  vector<ScalarImageWrapperBase *> layers;

  // A list of integrators that compute statistics for each image
  vector<ImageLabelIntegratorPointer> integrators;

  // Clear the list of column names
  m_ImageStatisticsColumnNames.clear();

  // Find all the images available for statistics computation
  for(LayerIterator it(id, MAIN_ROLE | OVERLAY_ROLE); !it.IsAtEnd(); ++it)
    {
    ScalarImageWrapperBase *lscalar = it.GetLayerAsScalar();
    if(lscalar)
      {
      m_ImageStatisticsColumnNames.push_back(lscalar->GetNickname());
      integrators.push_back(lscalar->GetLabelIntegrator(seg_space));
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

        lscalar = lvector->GetScalarRepresentation(SCALAR_REP_COMPONENT, j);
        integrators.push_back(lscalar->GetLabelIntegrator(seg_space));
        layers.push_back(lscalar);
        }
      }
    }

  // Get the number of gray image layers
  size_t ngray = layers.size();

  // Clear and initialize the statistics table
  m_Stats.clear();

  // Start the label image iteration
  using LabelImageType = LabelImageWrapper::ImageType;
  using RLLineIter = itk::ImageRegionConstIterator<LabelImageType::BufferType>;
  RLLineIter itLabel(seg->GetImage()->GetBuffer(), seg->GetImage()->GetBuffer()->GetBufferedRegion());

  // Iterate over the run lines and collect image statistics under each label
  itk::Index<3> runStart;
  for (; !itLabel.IsAtEnd(); ++itLabel)
  {
    // Index of the start of the line
    runStart[0] = seg->GetImage()->GetBufferedRegion().GetIndex(0);
    for(unsigned int i = 1; i < 3; i++)
      runStart[i] = itLabel.GetIndex()[i-1];

    // Get the RLE line
    const LabelImageType::RLLine &line = itLabel.Value();
    for (unsigned int i = 0; i < line.size(); i++)
    {
      LabelType label = line[i].second;

      auto &label_entry = m_Stats[label];
      if(label_entry.count == 0)
        label_entry.resize(ngray);

      for(size_t j = 0; j < ngray; j++)
      {
        integrators[j]->GetRunLengthIntensityStatistics(
          runStart, line[i].first, label_entry.nvalid[j], label_entry.sum[j], label_entry.sumsq[j]);
      }

      runStart[0] += line[i].first;
      label_entry.count += line[i].first;
    }
  }

  // Compute the size of a voxel, in mm^3
  const double *spacing = seg_space->GetSpacing().GetDataPointer();
  double volVoxel = spacing[0] * spacing[1] * spacing[2];
  
  // Compute the mean and standard deviation
  for(EntryMap::iterator it = m_Stats.begin(); it != m_Stats.end(); ++it)
    {
    Entry &entry = it->second;
    for(size_t j = 0; j < ngray; j++)
      {
      // The number of valid samples is fractional when samples fall on the
      // border of the image, and close to zero when a label lies (almost)
      // entirely outside of the image. In that case there is no meaningful
      // mean or standard deviation, and they are reported as NaN (n/a).
      double n = entry.nvalid[j];
      double mean = nan(""), stdev = nan("");
      if(n > 1e-6)
        {
        mean = entry.sum[j] / n;

        // The sample standard deviation needs more than one sample. Rounding
        // can make the sum of squared deviations slightly negative.
        if(n > 1.0)
          {
          double ss = std::max(0.0, entry.sumsq[j] - entry.sum[j] * mean);
          stdev = sqrt(ss / (n - 1.0));
          }
        }

      // Map with scale and shift
      entry.mean[j] = layers[j]->GetNativeIntensityMapping()->MapInternalToNative(mean);

      // Map with just shift
      entry.stdev[j] = layers[j]->GetNativeIntensityMapping()->MapGradientMagnitudeToNative(stdev);
      }
    entry.volume_mm3 = entry.count * volVoxel;
    }
}

void SegmentationStatistics
::GetVoxelCount(LabelVoxelCount &result, IRISApplication *app) const
{
  // Get selected segmentation layer
  LabelImageWrapper *liw = app->GetSelectedSegmentationLayer();

  // Get the iterator from the segmentation
  LabelImageWrapper::ConstIterator it = liw->GetImageConstIterator();

  // Iterate through image and update count
  LabelType runLabel = 0;
  unsigned long *cachedCnt = &result[runLabel];
  unsigned long runLength = 0;

  // Start counting
  for (; !it.IsAtEnd(); ++it, ++runLength)
    {
      LabelType label = it.Value();
      if (label != runLabel)
        {
          // std::cout << "run length: " << runLength << endl;
          *cachedCnt += runLength;

          // Reset for new count
          runLabel = label;
          cachedCnt = &result[runLabel];
          runLength = 0;
        }
    }

  // Add last run Length
  *cachedCnt += runLength;

  // Debug
  /*
  for (auto cit = result.cbegin(); cit != result.cend(); ++cit)
    {
      std::cout << cit->first << "=>" << cit->second << std::endl;
    }
    */
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
     
      for(size_t j = 0; j < m_Stats[i].mean.size(); j++)
        {
        fout << " / " << std::internal << std::setw(10) 
          << m_Stats[i].mean[j] << " / " << std::setw(10)
          << m_Stats[i].stdev[j];
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

    for(int j = 0; j < entry.mean.size(); j++)
      {
      oss << colsep << entry.mean[j];
      oss << colsep << entry.stdev[j];
      }

    oss << std::endl;
    }
}

