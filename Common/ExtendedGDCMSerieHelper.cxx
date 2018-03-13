/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ExtendedGDCMSerieHelper.cxx,v $
  Language:  C++
  Date:      $Date: 2010/10/14 16:21:04 $
  Version:   $Revision: 1.6 $
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
#include "ExtendedGDCMSerieHelper.h"
#include "IRISException.h"

#include "gdcmImageHelper.h"

using namespace gdcm;

ExtendedGDCMSerieHelper::ExtendedGDCMSerieHelper()
 : gdcm::SerieHelper()
{
}

void ExtendedGDCMSerieHelper
::SetFilesAndOrder(std::vector<std::string> &files, int &n_images_per_ipp)
{
  this->Clear();
  this->SetUseSeriesDetails(true);

  // Typically there will be just one image per ipp
  n_images_per_ipp = 1;

  for(int i = 0; i < files.size(); i++)
    this->AddFileName(files[i]);

  FileList *flist = this->GetFirstSingleSerieUIDFileSet();

  if(flist->size() != files.size())
    throw(IRISException(
        "Mismatch in number of DICOM files parsed (%d vs. %d). "),
      flist->size(), files.size());

  // Order the file list - by position or by filename
  if (!IPPMultiOrdering( flist, n_images_per_ipp))
    {
    this->FileNameOrdering(flist );
    }

  files.clear();
  for(int i = 0; i < flist->size(); i++)
    {
    FileWithName *header = (*flist)[i];
    files.push_back(header->filename);
    }
}


bool
ExtendedGDCMSerieHelper
::IPPMultiOrdering( FileList *fileList, int &n_images_per_ipp )
{
  //iop is calculated based on the file file
  std::vector<double> cosines;
  double normal[3] = {};
  std::vector<double> ipp;
  double dist;
  double min = 0, max = 0;
  bool first = true;

  // Initialize the return value to zero
  n_images_per_ipp = -1;

  // Duplicate the file list and sort it by filename first
  FileList fileListPreSorted = *fileList;
  std::sort(fileListPreSorted.begin(), fileListPreSorted.end(), ExtendedGDCMSerieHelper::FileNameSortPredicate);

  std::multimap<double,SmartPointer<FileWithName> > distmultimap;
  // Use a multimap to sort the distances from 0,0,0
  for ( FileList::const_iterator
    it = fileListPreSorted.begin();
    it != fileListPreSorted.end(); ++it )
    {
    if ( first )
      {
      //(*it)->GetImageOrientationPatient( cosines );
      cosines = ImageHelper::GetDirectionCosinesValue( **it );

      // You only have to do this once for all slices in the volume. Next,
      // for each slice, calculate the distance along the slice normal
      // using the IPP ("Image Position Patient") tag.
      // ("dist" is initialized to zero before reading the first slice) :
      normal[0] = cosines[1]*cosines[5] - cosines[2]*cosines[4];
      normal[1] = cosines[2]*cosines[3] - cosines[0]*cosines[5];
      normal[2] = cosines[0]*cosines[4] - cosines[1]*cosines[3];

      ipp = ImageHelper::GetOriginValue( **it );
      //ipp[0] = (*it)->GetXOrigin();
      //ipp[1] = (*it)->GetYOrigin();
      //ipp[2] = (*it)->GetZOrigin();

      dist = 0;
      for ( int i = 0; i < 3; ++i )
        {
        dist += normal[i]*ipp[i];
        }

      distmultimap.insert(std::pair<const double,SmartPointer<FileWithName> >(dist, *it));

      max = min = dist;
      first = false;
      }
    else
      {
      ipp = ImageHelper::GetOriginValue( **it );
      //ipp[0] = (*it)->GetXOrigin();
      //ipp[1] = (*it)->GetYOrigin();
      //ipp[2] = (*it)->GetZOrigin();

      dist = 0;
      for ( int i = 0; i < 3; ++i )
        {
        dist += normal[i]*ipp[i];
        }

      distmultimap.insert(std::pair<const double,SmartPointer<FileWithName> >(dist, *it));

      min = (min < dist) ? min : dist;
      max = (max > dist) ? max : dist;
      }
    }

  // Find out if min/max are coherent
  if ( min == max )
    {
    gdcmWarningMacro("Looks like all images have the exact same image position"
      << ". No PositionPatientOrdering sort performed" );
    return false;
    }

  // Check to see if image shares a common position
  bool ok = true;
  for (std::multimap<double, SmartPointer<FileWithName> >::iterator it2 = distmultimap.begin();
    it2 != distmultimap.end();
    ++it2)
    {
    int count = distmultimap.count((*it2).first);
    if (n_images_per_ipp < 0)
      {
      n_images_per_ipp = count;
      }
    else if(count != n_images_per_ipp)
      {
      gdcmErrorMacro("Non-uniform number of images per IPP");
      ok = false;
      break;
      }
    }
  if (!ok)
    {
    n_images_per_ipp = -1;
    return false;
    }

  fileList->clear();  // doesn't delete list elements, only nodes

  for (std::multimap<double, SmartPointer<FileWithName> >::iterator it3 = distmultimap.begin();
       it3 != distmultimap.end();
       ++it3)
    {
    fileList->push_back( (*it3).second );
    }

  distmultimap.clear();
  return true;
}
