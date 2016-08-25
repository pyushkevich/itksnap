/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: IRISVectorTypes.h,v $
  Language:  C++
  Date:      $Date: 2008/11/01 11:32:00 $
  Version:   $Revision: 1.3 $
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

  -----

  Copyright (c) 2003 Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef IMAGEFUNCTIONS_H
#define IMAGEFUNCTIONS_H

#include "IRISVectorTypes.h"
#include "itkImageBase.h"

namespace itk { template <unsigned int D> class ImageBase; }

/*
 * Miscellaneous functions involving ITK images
 */

template <unsigned int VDim, typename TVector>
void GetImagePhysicalExtents(itk::ImageBase<VDim> *image,
                             TVector &ext_min,
                             TVector &ext_max)
{
  // Get the index of each corner of the image
  itk::ImageRegion<VDim> lpr = image->GetLargestPossibleRegion();

  int n_corners = 1 << VDim;
  for(int i = 0; i < n_corners; i++)
    {
    // Compute the index of this corner
    itk::ContinuousIndex<double, VDim> index;
    for(int j = 0; j < VDim; j++)
      {
      index[j] = ((i >> j) & 1)
                 ? lpr.GetIndex()[j] - 0.5
                 : lpr.GetIndex()[j] + lpr.GetSize()[j] - 0.5;
      }

    // Map this corner to physical space
    itk::Point<double, VDim> point;
    image->TransformContinuousIndexToPhysicalPoint(index, point);

    // Update min/max with this point
    for(int j = 0; j < VDim; j++)
      {
      if(i == 0 || ext_min[j] > point[j])
        ext_min[j] = point[j];

      if(i == 0 || ext_max[j] < point[j])
        ext_max[j] = point[j];
      }
    }
}



#endif // IMAGEFUNCTIONS_H
