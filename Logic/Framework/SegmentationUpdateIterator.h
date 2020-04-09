/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: GenericImageData.h,v $
  Language:  C++
  Date:      $Date: 2009/08/29 23:02:43 $
  Version:   $Revision: 1.11 $
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
#ifndef __SegmentationUpdateIterator_h_
#define __SegmentationUpdateIterator_h_

#include "SNAPCommon.h"
#include "ImageWrapperTraits.h"
#include "UndoDataManager.h"


/**
 * \class SegmentationUpdate
 * \brief This class handles updates to the segmentation image at a high level.
 */
class SegmentationUpdateIterator
{
public:
  typedef itk::Index<3>                                        IndexType;
  typedef itk::ImageRegion<3>                                  RegionType;
  typedef LabelImageWrapper::ImageType                         LabelImageType;
  typedef itk::ImageRegionIterator<LabelImageType>             LabelIteratorType;

  typedef UndoDataManager<LabelType>::Delta                    UndoDelta;

  enum UpdateType {
    FOREGROUND, BACKGROUND, SKIP
  };

  SegmentationUpdateIterator(LabelImageType *labelImage,
                             const RegionType &region,
                             LabelType active_label,
                             DrawOverFilter draw_over)
    : m_Region(region),
      m_ActiveLabel(active_label),
      m_DrawOver(draw_over),
      m_Iterator(labelImage, region),
      m_ChangedVoxels(0)
  {
    // Create the delta
    m_Delta = new UndoDelta();
    m_Delta->SetRegion(region);

    // Set the voxel delta to zero
    m_VoxelDelta = 0;
  }

  ~SegmentationUpdateIterator()
  {
    if(m_Delta)
      delete m_Delta;
  }

  void operator ++()
  {
    // Encode the current voxel delta
    m_Delta->Encode(m_VoxelDelta);
    m_VoxelDelta = 0;

    // Keep track of changed voxels
    if(m_VoxelDelta != 0)
      m_ChangedVoxels++;

    // Update the internal iterator
    ++m_Iterator;
  }

  const IndexType GetIndex()
  {
    return m_Iterator.GetIndex();
  }

  /**
   * Paint with a specified label - respecting the draw-over mask
   */
  virtual void PaintLabel(LabelType new_label)
  {
    LabelType lOld = m_Iterator.Get();

    if(m_DrawOver.CoverageMode == PAINT_OVER_ALL ||
       (m_DrawOver.CoverageMode == PAINT_OVER_ONE && lOld == m_DrawOver.DrawOverLabel) ||
       (m_DrawOver.CoverageMode == PAINT_OVER_VISIBLE && lOld != 0))
      {
      if(lOld != new_label)
        {
        m_VoxelDelta += new_label - lOld;
        m_Iterator.Set(new_label);
        m_ChangedVoxels++;
        }
      }
  }


  /**
   * Default painting mode - applies active label using the current draw over mask
   */
  void PaintAsForeground()
  {
    this->PaintLabel(m_ActiveLabel);
  }

  /**
   * Similar but clear label is not affected (used in cutplane mode)
   */
  void PaintAsForegroundPreserveClear()
  {
    LabelType lOld = m_Iterator.Get();
    if(lOld == 0)
      return;

    if(m_DrawOver.CoverageMode == PAINT_OVER_ALL ||
       (m_DrawOver.CoverageMode == PAINT_OVER_ONE && lOld == m_DrawOver.DrawOverLabel) ||
       (m_DrawOver.CoverageMode == PAINT_OVER_VISIBLE && lOld != 0))
      {
      if(lOld != m_ActiveLabel)
        {
        m_VoxelDelta += m_ActiveLabel - lOld;
        m_Iterator.Set(m_ActiveLabel);
        m_ChangedVoxels++;
        }
      }
  }



  /**
   * Reverse painting mode - applies clear label over active label (paintbrush RMB click)
   */
  void PaintAsBackground()
  {
    LabelType lOld = m_Iterator.Get();

    if(m_ActiveLabel != 0 && lOld == m_ActiveLabel)
      {
      m_VoxelDelta += 0 - lOld;
      m_Iterator.Set(0);
      m_ChangedVoxels++;
      }
  }

  /**
   * More general method, replaces label with new_label if current label matches target_label,
   * does not take into account Active/DrawOver state
   */
  void ReplaceLabel(LabelType target_label, LabelType new_label)
  {
    LabelType lOld = m_Iterator.Get();

    if(lOld == target_label)
      {
      m_VoxelDelta += new_label - lOld;
      m_Iterator.Set(new_label);
      m_ChangedVoxels++;
      }
  }

  /**
   * A more funky paint method, applies the following test, where X is the
   * label at the voxel
   *
   * if(X != A and X \in DrawOver)
   *   X = B
   */
  void PaintLabelWithExtraProtection(LabelType protect_label, LabelType new_label)
  {
    LabelType lOld = m_Iterator.Get();
  
    // Test for protection or empty operation
    if(lOld == protect_label || lOld == new_label)
      return;

    // Apply the draw over test
    if(m_DrawOver.CoverageMode == PAINT_OVER_ALL ||
       (m_DrawOver.CoverageMode == PAINT_OVER_ONE && lOld == m_DrawOver.DrawOverLabel) ||
       (m_DrawOver.CoverageMode == PAINT_OVER_VISIBLE && lOld != 0))
      {
      m_VoxelDelta += new_label - lOld;
      m_Iterator.Set(new_label);
      m_ChangedVoxels++;
      }
  }


  bool IsAtEnd()
  {
    return m_Iterator.IsAtEnd();
  }

  /**
   * Call this method at the end of the iteration to finish encoding. This will also set the
   * modified flag of the label image if there were any actual updates.
   */
  void Finalize()
  {
    m_Delta->FinishEncoding();
    if(m_ChangedVoxels > 0)
      m_Iterator.GetImage()->Modified();
  }

  // Keep delta from being deleted
  UndoDelta *RelinquishDelta()
  {
    UndoDelta *delta = m_Delta;
    m_Delta = NULL;
    return delta;
  }

  // Get the number of changed voxels
  unsigned long GetNumberOfChangedVoxels() const
  {
    return m_ChangedVoxels;
  }

  // Get the pointer to the detla
  UndoDelta *GetDelta() const
  {
    return m_Delta;
  }

protected:

  // Name of the segmentation update (for undo tracking)
  std::string m_Title;

  // Region over which update is performed
  RegionType m_Region;

  // Active label for the registration update
  LabelType m_ActiveLabel;

  // Coverage mode
  DrawOverFilter m_DrawOver;

  // RLE encoding of the segmentation update - for storing undo/redo points
  UndoDelta *m_Delta;

  // Iterator used internally
  LabelIteratorType m_Iterator;

  // Delta at the current location
  LabelType m_VoxelDelta;

  // Number of voxels actually modified
  unsigned long m_ChangedVoxels;
};


#endif // SegmentationUpdateIterator
