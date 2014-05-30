/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ImageCoordinateGeometry.h,v $
  Language:  C++
  Date:      $Date: 2008/11/15 12:20:38 $
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

=========================================================================*/
#ifndef __ImageCoordinateGeometry_h_
#define __ImageCoordinateGeometry_h_

#include "ImageCoordinateTransform.h"
#include "IRISDisplayGeometry.h"
#include <map>


/**
 * \class ImageCoordinateGeometry
 * \brief This class describes the geometric relationship between the image 
 * coordinate system, the patient coordinate system, and the display coordinate
 * system.
 */
class ImageCoordinateGeometry
  {
public:
  typedef vnl_matrix<double> DirectionMatrix;

  /**
   * @brief Enum describing the directions of image axes in anatomical space
   */
  enum AxisDirection {
    INVALID_DIRECTION = 0,
    R_TO_L = 1, L_TO_R = -1,
    A_TO_P = 2, P_TO_A = -2,
    I_TO_S = 3, S_TO_I = -3
  };

  /** A map from AxisDirection enum to user readable strings */
  typedef std::map<AxisDirection, std::string> AxisDirectionDescriptionMap;

  /** Constructor initializes geometry with default identity transforms */
  ImageCoordinateGeometry();

  /** Constructor that calls GetGeometry */
  ImageCoordinateGeometry(DirectionMatrix imageDirection,
                          const IRISDisplayGeometry &dispGeom,
                          const Vector3ui &imageSize);

  /** Virtual destructor */
  virtual ~ImageCoordinateGeometry() {}

  /** Initializes geometry with tranforms specified by character
   * RAI codes for the image-anatomy and display-anatomy transforms and the 
   * image size */
  void SetGeometry(DirectionMatrix imageDirection,
                   const IRISDisplayGeometry &dispGeom,
                   const Vector3ui &imageSize);

  /** Get the transform from image to patient coordinate system */
  irisGetMacro(ImageToAnatomyTransform,const ImageCoordinateTransform &)

  /** Get the image to anatomy direction matrix */
  irisGetMacro(ImageDirectionCosineMatrix, DirectionMatrix)

  /** Get the transform from patient to display coordinate system */
  const ImageCoordinateTransform & GetAnatomyToDisplayTransform(unsigned int i) const
  {
    return m_AnatomyToDisplayTransform[i];
  }

  /** Get the transform from image to display coordinate system */
  const ImageCoordinateTransform & GetImageToDisplayTransform(unsigned int i) const
  {
    return m_ImageToDisplayTransform[i];
  }

  /** Get the transform from display to image coordinate system */  
  const ImageCoordinateTransform & GetDisplayToImageTransform(unsigned int i) const
  {
    return m_DisplayToImageTransform[i];
  }

  /** Get the display to anatomy RAIs */
  const std::string & GetDisplayToAnatomyRAI(unsigned int i) const
  {
    return m_DisplayGeometry.DisplayToAnatomyRAI[i];
  }

  /** Check an RAI orientation code for validity */
  static bool IsRAICodeValid(const std::string &rai);

  /** Get the descriptions of the axis directions */
  static AxisDirectionDescriptionMap &GetAxisDirectionDescriptionMap();

  /** Convert an RAI code letter to a axis direction */
  static AxisDirection ConvertRAILetterToAxisDirection(char letter);

  /** Convert axis direction to RAI letter */
  static char ConvertAxisDirectionToRAILetter(AxisDirection dir);

  /** Map an RAI code to a mapping vector of positive or negative 1,2,3 */
  static Vector3i ConvertRAIToCoordinateMapping(const std::string &rai);

  /** Map ITK direction cosines matrix to the closest RAI code */
  static std::string ConvertDirectionMatrixToClosestRAICode(DirectionMatrix mat);

  /** Map RAI code to an ITK direction cosines matrix */
  static DirectionMatrix ConvertRAICodeToDirectionMatrix(const std::string &rai);

  /** Check if the direction cosines are oblique (not parallel to coord system) */
  static bool IsDirectionMatrixOblique(DirectionMatrix mat);

  /** Invert a mapping vector of 1,2,3 */
  static Vector3i InvertMappingVector(const Vector3i &mapping);

private:

  // Slice transform information
  ImageCoordinateTransform m_ImageToAnatomyTransform;
  ImageCoordinateTransform m_AnatomyToDisplayTransform[3];

  // Three anatomy to display transforms (for each of the 3D slices)
  ImageCoordinateTransform m_ImageToDisplayTransform[3];
  ImageCoordinateTransform m_DisplayToImageTransform[3];

  // The string representation of the display to anatomy RAI codes
  IRISDisplayGeometry m_DisplayGeometry;

  // Image to anatomy direction matrix
  DirectionMatrix m_ImageDirectionCosineMatrix;  

  // RAI codes
  static const char m_RAICodes[3][2];

  // The axis direction to string map
  static AxisDirectionDescriptionMap m_AxisDirectionDescriptionMap;
};

#endif
