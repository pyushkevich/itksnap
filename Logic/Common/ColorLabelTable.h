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
#include "itkDataObject.h"
#include "SNAPEvents.h"
#include "itkObjectFactory.h"
#include "itkTimeStamp.h"

/**
 * \class ColorLabelTable
 * \brief A table for managing color labels
 */
class ColorLabelTable : public itk::DataObject
{
public:
  // Standard ITK macros
  irisITKObjectMacro(ColorLabelTable, Object)

  // Events that we fire
  FIRES(SegmentationLabelChangeEvent)

  // Flat file IO
  void LoadFromFile(const char *file) throw(itk::ExceptionObject);
  void SaveToFile(const char *file) const throw(itk::ExceptionObject);

  // Registry IO
  void LoadFromRegistry(Registry &registry);
  void SaveToRegistry(Registry &registry) const;

  /** Initialize the labels to a default state */
  void InitializeToDefaults();

  /** Clear all the color labels, except the 'clear' label */
  void RemoveAllLabels();

  /** Get the number of valid color labels */
  size_t GetNumberOfValidLabels() const;

  /** Get the first valid non-zero color label (or zero if there are none)  */
  LabelType GetFirstValidLabel() const;

  /**
    Get the first insertion spot after the specified color label. Return of
    0 signifies no room left for insertion. The method wraps around the list
    of labels.
   */
  LabelType GetInsertionSpot(LabelType pos);

  /**
    Find the next valid label, given the selected label. If all else fails,
    this will return label 0. The method wraps around.
    */
  LabelType FindNextValidLabel(LabelType pos, bool includeClearInSearch);

  /**
    Get the color label corresponding to the given value. If the requested
    label is not valid, a default value is returned
  */
  const ColorLabel GetColorLabel(size_t id) const;

  /** Set the i'th color label, i between 0 and 255 */
  void SetColorLabel(size_t id, const ColorLabel &label);

  /** Generate a default color label at index i */
  static ColorLabel GetDefaultColorLabel(LabelType id);

  bool IsColorLabelValid(LabelType id) const;

  /** Sets the color label valid or invalid. During invalidation, the label
   * reverts to default values */
  void SetColorLabelValid(LabelType id, bool flag);

  // Map between valid label IDS and label descriptors
  typedef std::map<LabelType, ColorLabel> ValidLabelMap;
  typedef ValidLabelMap::const_iterator ValidLabelConstIterator;

  /** Get the iterator over the valid labels */
  ValidLabelConstIterator begin() const { return m_LabelMap.begin(); }
  ValidLabelConstIterator end() const { return m_LabelMap.end(); }

  /** Get the collection of defined/valid labels */
  const ValidLabelMap &GetValidLabels() const { return m_LabelMap; }

protected:

  ColorLabelTable();
  virtual ~ColorLabelTable() {}

  // The main data array
  ValidLabelMap m_LabelMap;

  // A flat array of color labels
  // ColorLabel m_Label[MAX_COLOR_LABELS], m_DefaultLabel[MAX_COLOR_LABELS];

  static const char *m_ColorList[];
  static const size_t m_ColorListSize;
};











#endif
