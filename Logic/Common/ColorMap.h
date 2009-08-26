/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ColorMap.h,v $
  Language:  C++
  Date:      $Date: 2009/08/26 21:49:56 $
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

#ifndef __ColorMap_h_
#define __ColorMap_h_

#include "itkRGBAPixel.h"
#include <vector>

/**
 * \class ColorMap
 * \brief This class provides a representation of color maps. 
 * A color map is a mapping from the interval [0,1] to the RGBA space. The
 * mapping is piecewise continuous. Where it is continuous, it is linear.
 *
 * The color map is represented as an ordered list of points. Each point has
 * an index value (between 0 and 1). Points may be continuous (i.e., the color
 * map is continuous at the point) and discontinuous. A continuous point has 
 * one RGBA value, and discontinuous points have two values. In between points,
 * the color map is interpolated linearly.
 */
class ColorMap 
{
  public:

    /** RGBA type */
    typedef unsigned char EltType;
    typedef itk::RGBAPixel<EltType> RGBAType;

    /** Type of point */
    enum PointType { CONTINUOUS, DISCONTINUOUS };

    /** System presets */
    enum SystemPreset {
      COLORMAP_GREY = 0, COLORMAP_RED, COLORMAP_GREEN, COLORMAP_BLUE, 
      COLORMAP_HOT, COLORMAP_COOL, COLORMAP_SPRING, COLORMAP_SUMMER, 
      COLORMAP_AUTUMN, COLORMAP_WINTER, COLORMAP_COPPER, COLORMAP_HSV, 
      COLORMAP_JET, COLORMAP_SIZE
    };

    /** Point representation */
    struct Point 
    {
      double m_Index;
      PointType m_Type;
      RGBAType m_RGBA[2];

      Point();

      // Continuous point
      Point(double, EltType r, EltType g, EltType b, EltType a);

      // Point with alpha discontinuity
      Point(double, EltType r, EltType g, EltType b, EltType a1, EltType a2);

      // Point with full discontinuity
      Point(double, 
          EltType r1, EltType g1, EltType b1, EltType a1, 
          EltType r2, EltType g2, EltType b2, EltType a2);
    };

    /** 
     * This method maps a value to an RGBA tuple. Values less than 0 and greater
     * than one are accepted, and are mapped to 0 and 1, respecitively. 
     */
    RGBAType MapIndexToRGBA(double j) const;

    /** This method initializes the color map to one of the system presets */
    void SetToSystemPreset(SystemPreset preset);

    size_t GetNumberOfPoints()
      { return m_Points.size(); }

    Point GetPoint(size_t j)
      { return m_Points[j]; }

    void UpdatePoint(size_t j, const Point &p)
      { m_Points[j] = p; }

  private:

    typedef std::vector<Point> PointList;
    typedef PointList::iterator PointIterator;
    typedef PointList::const_iterator PointConstIterator;

    // Array of points, which must always be sorted by the index
    PointList m_Points;



};


#endif 


