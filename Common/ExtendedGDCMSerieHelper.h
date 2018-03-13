/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ExtendedGDCMSerieHelper.h,v $
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
#ifndef __ExtendedGDCMSerieHelper_h_
#define __ExtendedGDCMSerieHelper_h_

#include "gdcmSerieHelper.h"


/**
 * This is basically to just expose AddFileName
 */
class ExtendedGDCMSerieHelper : public gdcm::SerieHelper
{
public:
  ExtendedGDCMSerieHelper();

  void SetFilesAndOrder(std::vector<std::string> &files, int &n_images_per_ipp);

protected:
  bool IPPMultiOrdering(gdcm::FileList *fileList, int &n_images_per_ipp);

  static bool FileNameSortPredicate(
      const gdcm::SmartPointer<gdcm::FileWithName>& d1,
      const gdcm::SmartPointer<gdcm::FileWithName>& d2)
  {
    return d1->filename < d2->filename;
  }
};


#endif // __ExtendedGDCMSerieHelper_h_
