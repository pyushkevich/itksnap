/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ColorLabel.h,v $
  Language:  C++
  Date:      $Date: 2008/11/17 19:38:23 $
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
#ifndef __ColorLabel_h_
#define __ColorLabel_h_

#include "SNAPCommon.h"
#include "itkTimeStamp.h"

#include <assert.h>
#include <list>
    
/**
 * \class ColorLabel
 * \brief Information about a label used for segmentation.
 * Color labels used to describe pixels in the segmented
 * image.  These labels correspond to the intensities in the
 * segmentation image in IRISImageData class
 */
class ColorLabel {
public:
  // Dummy constructor and destructor (to make gcc happy)
  ColorLabel() { m_TimeStamp.Modified(); }
  virtual ~ColorLabel() {}

  // Copy constructor
  ColorLabel(const ColorLabel &cl)
    : m_Label(cl.m_Label), m_Value(cl.m_Value), m_DatabaseId(cl.m_DatabaseId),
      m_Id(cl.m_Id), m_Visible(cl.m_Visible), m_VisibleIn3D(cl.m_VisibleIn3D),
      m_UpdateTime(cl.m_UpdateTime), m_Alpha(cl.m_Alpha)
  {
    m_RGB[0] = cl.m_RGB[0];
    m_RGB[1] = cl.m_RGB[1];
    m_RGB[2] = cl.m_RGB[2];
    m_TimeStamp = cl.m_TimeStamp;
  }

  // Read the Visible attribute
  irisIsMacro(Visible);

  // Set the Visible attribute
  irisSetMacro(Visible,bool);

  // Read the Valid attribute
  // irisIsMacro(Valid);

  // Set the Valid attribute
  // irisSetMacro(Valid,bool);

  // Read the DoMesh attribute
  irisIsMacro(VisibleIn3D);

  // Set the DoMesh attribute
  irisSetMacro(VisibleIn3D,bool);

  // Read the Label attribute
  virtual const char *GetLabel() const
    { return m_Label.c_str(); }

  // Set the Label attribute
  irisSetMacro(Label,const char *);

  // Read the Alpha attribute
  irisGetMacro(Alpha,unsigned char);

  // Set the Alpha attribute
  irisSetMacro(Alpha,unsigned char);

  // Get the value
  irisGetMacro(Value, LabelType);

  // Check Opaqueness
  bool IsOpaque() const 
    { return m_Alpha == 255; }

  // Read the RGB attributes
  unsigned char GetRGB(unsigned int index) const {
    assert(index < 3);
    return m_RGB[index];
  }

  // Set the RGB attributes
  void SetRGB(unsigned int index, unsigned char in_Value) {
    assert(index < 3);
    m_RGB[index] = in_Value;
  }

  // Set all three at once
  void SetRGB(unsigned char in_Red,unsigned char in_Green,unsigned char in_Blue) {
    m_RGB[0] = in_Red;
    m_RGB[1] = in_Green;
    m_RGB[2] = in_Blue;
  }

  // Get the RGB values as a double vector (range 0-1)
  Vector3d GetRGBAsDoubleVector() const
  {
    return Vector3d(m_RGB[0] / 255., m_RGB[1] / 255., m_RGB[2] / 255.);
  }

  // Copy RGB into an array
  void GetRGBVector(unsigned char array[3]) const {
    array[0] = m_RGB[0];
    array[1] = m_RGB[1];
    array[2] = m_RGB[2];
  }

  // Copy RGB into an array
  void SetRGBVector(const unsigned char array[3]) {
    m_RGB[0] = array[0];
    m_RGB[1] = array[1];
    m_RGB[2] = array[2];
  }

  // Copy RGB into an array
  void GetRGBAVector(unsigned char array[4]) const {
    array[0] = m_RGB[0];
    array[1] = m_RGB[1];
    array[2] = m_RGB[2];
    array[3] = m_Alpha;
  }

  // Copy RGB into an array
  void SetRGBAVector(const unsigned char array[4]) {
    m_RGB[0] = array[0];
    m_RGB[1] = array[1];
    m_RGB[2] = array[2];
    m_Alpha  = array[3];
  }

  // Copy all properties, except for the label id and the valid flag 
  // from one label to the other. This is used to reassign ids to labels
  // because the labels are in sequential order
  void SetPropertiesFromColorLabel(const ColorLabel &lSource)
    {
    m_VisibleIn3D = lSource.m_VisibleIn3D;
    m_Visible = lSource.m_Visible;
    m_Alpha = lSource.m_Alpha;
    m_Label = lSource.m_Label;
    m_RGB[0] = lSource.m_RGB[0];
    m_RGB[1] = lSource.m_RGB[1];
    m_RGB[2] = lSource.m_RGB[2];
    }

  const itk::TimeStamp &GetTimeStamp() const { return m_TimeStamp; }
  itk::TimeStamp &GetTimeStamp() { return m_TimeStamp; }

private:
  // The descriptive text of the label
  std::string m_Label;

  // The intensity assigned to the label in the segmentation image
  LabelType m_Value;

  // The ID of the label in some external database.
  std::string m_DatabaseId;

  // The internal ID of the label. By default it's equal to the value
  unsigned int m_Id;

  // Whether the label is visible in 2D views
  bool m_Visible;

  // Whether the mesh for the label is computed
  bool m_VisibleIn3D;

  // The system timestamp when the data marked with this label was last updated
  unsigned long m_UpdateTime;

  // Whether the label is valid (has been added)
  // bool m_Valid;

  // The transparency level of the label
  unsigned char m_Alpha;

  // The color of the label
  unsigned char m_RGB[3];

  // A list of labels that contain this label
  std::list< unsigned int > m_Parents;

  // A list of labels that this label contains
  std::list< unsigned int > m_Children;

  // An itk TimeStamp, allows tracking of changes to the label appearance
  itk::TimeStamp m_TimeStamp;
};


#endif
