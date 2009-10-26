/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ColorLabelTable.h,v $
  Language:  C++
  Date:      $Date: 2009/10/26 22:18:03 $
  Version:   $Revision: 1.5 $
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
#ifndef __ColorLabelTable_h_
#define __ColorLabelTable_h_

#include "Registry.h"
#include "ColorLabel.h"
#include "itkExceptionObject.h"

/**
 * \class ColorLabelTable
 * \brief A table for managing color labels
 */
class ColorLabelTable
{
public:
  ColorLabelTable();

  // Flat file IO
  void LoadFromFile(const char *file) throw(itk::ExceptionObject);
  void SaveToFile(const char *file) throw(itk::ExceptionObject);

  // Registry IO
  void LoadFromRegistry(Registry &registry);
  void SaveToRegistry(Registry &registry);

  /** Initialize the labels to a default state */
  void InitializeToDefaults();

  /** Clear all the color labels, except the 'clear' label */
  void RemoveAllLabels();

  /** Get the number of valid color labels */
  size_t GetNumberOfValidLabels();

  /** Get the i'th label, i between 0 and 255 */
  const ColorLabel &GetColorLabel(size_t id) const
    {
    assert(id < MAX_COLOR_LABELS);
    return m_Label[id];
    } 

  /** Set the i'th color label, i between 0 and 255 */
  void SetColorLabel(size_t id, const ColorLabel &label)
    { 
    assert(id < MAX_COLOR_LABELS);
    m_Label[id] = label; 
    }

  /** Generate a default color label at index i */
  ColorLabel GetDefaultColorLabel(size_t id)
    { 
    assert(id < MAX_COLOR_LABELS);
    return m_DefaultLabel[id]; 
    }

  bool IsColorLabelValid(size_t id) const
    { return GetColorLabel(id).IsValid(); }

  /** Sets the color label valid or invalid. During invalidation, the label
   * reverts to default values */
  void SetColorLabelValid(size_t id, bool flag);

  /** Return the first valid color label, or zero if there aren't any */
  size_t GetFirstValidLabel() const;

private:
  // A flat array of color labels
  ColorLabel m_Label[MAX_COLOR_LABELS], m_DefaultLabel[MAX_COLOR_LABELS];

  static const char *m_ColorList[];
  static const size_t m_ColorListSize;
};

#endif
