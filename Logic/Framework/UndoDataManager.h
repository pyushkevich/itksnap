/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: UndoDataManager.h,v $
  Language:  C++
  Date:      $Date: 2008/11/15 12:20:38 $
  Version:   $Revision: 1.4 $
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
#ifndef __UndoDataManager_h_
#define __UndoDataManager_h_

#include <vector>
#include <list>

#include <RLEImage.h>

/**
 * The Delta class represents a difference between two images used in
 * the Undo system. It only supports linear traversal of images and
 * stores differences in an RLE (run length encoding) format.
 */
template <typename TPixel>
class UndoDelta
{
public:
  typedef itk::ImageRegion<3> RegionType;

  UndoDelta();

  void SetRegion(const RegionType &region)
  { this->m_Region = region; }

  const RegionType &GetRegion()
  { return m_Region; }

  void Encode(const TPixel &value);

  void FinishEncoding();

  size_t GetNumberOfRLEs()
  { return m_Array.size(); }

  TPixel GetRLEValue(size_t i)
  { return m_Array[i].second; }

  size_t GetRLELength(size_t i)
  { return m_Array[i].first; }

  unsigned long GetUniqueID() const
  { return m_UniqueID; }

  UndoDelta & operator = (const UndoDelta &other);

protected:
  typedef std::pair<size_t, TPixel> RLEPair;
  typedef std::vector<RLEPair> RLEArray;
  RLEArray m_Array;
  size_t m_CurrentLength;
  TPixel m_LastValue;

  // The delta is associated with an image region
  RegionType m_Region;

  // Each delta is assigned a unique ID at creation
  unsigned long m_UniqueID;
  static unsigned long m_UniqueIDCounter;
};


/**
 * \class UndoDataManager
 * \brief Manages data (delta updates) for undo/redo in itk-snap
 */
template<typename TPixel> class UndoDataManager
{
public:

  typedef itk::ImageRegion<3> RegionType;



  /** List of deltas and related iterators */
  typedef UndoDelta<TPixel> Delta;
  typedef std::list<Delta *> DList;
  typedef typename DList::iterator DIterator;
  typedef typename DList::const_iterator DConstIterator;

  /**
   * A "commit" to the undo system consists of one or more deltas. For example
   * in paintbrush drawing, as you drag the paintbrush, deltas are generated, but
   * these deltas are associated with a single commit. The commit owns the deltas
   * and deletes them when it is itself deleted.
   */
  class Commit
  {
  public:
    Commit(const DList &list, const char *name);
    void DeleteDeltas();
    size_t GetNumberOfRLEs() const;
    const DList &GetDeltas() const { return m_Deltas; }
  protected:
    DList m_Deltas;
    std::string m_Name;
  };

  UndoDataManager(size_t nMinCommits, size_t nMaxTotalSize);

  /** Add a delta to the staging list. The staging list must be committed */
  void AddDeltaToStaging(Delta *delta);

  /** Commit the deltas in the staging list - returns total number of RLEs updated */
  int CommitStaging(const char *text);

  /** Clear the undo stack (removes all commits) */
  void Clear();

  bool IsUndoPossible();
  const Commit &GetCommitForUndo();

  bool IsRedoPossible();
  const Commit &GetCommitForRedo();

  size_t GetNumberOfCommits()
    { return m_CommitList.size(); }

private:

  // Current staging list - where deltas are added
  DList m_StagingList;

  // List of commits typedefs
  typedef std::list<Commit> CList;
  typedef typename CList::iterator CIterator;
  typedef typename CList::const_iterator CConstIterator;

  // A list of commits
  CList m_CommitList;
  CIterator m_Position;
  size_t m_TotalSize, m_MinCommits, m_MaxTotalSize;
};

#endif // __UndoDataManager_h_
