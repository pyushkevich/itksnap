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
#include <vector>
#include <list>


/**
 * \class UndoDataManager
 * \brief Manages data (delta updates) for undo/redo in itk-snap
 */
template<typename TPixel> class UndoDataManager
{
public:

  /**
   * The Delta class represents a difference between two images used in
   * the Undo system. It only supports linear traversal of images and
   * stores differences in an RLE (run length encoding) format.
   */
  class Delta 
    {
    public:
      Delta()
        {
        m_CurrentLength = 0;
        m_UniqueID = m_UniqueIDCounter++;
        }
      
      void Encode(const TPixel &value)
        {
        if(m_CurrentLength == 0)
          {
          m_LastValue = value;
          m_CurrentLength = 1;
          }
        else if(value == m_LastValue)
          {
          m_CurrentLength++;
          }
        else
          {
          m_Array.push_back(std::make_pair(m_CurrentLength, m_LastValue));
          m_CurrentLength = 1;
          m_LastValue = value;
          }
        }

      void FinishEncoding()
        {
        if(m_CurrentLength > 0)
          m_Array.push_back(std::make_pair(m_CurrentLength, m_LastValue));
        }

      size_t GetNumberOfRLEs()
        { return m_Array.size(); }

      TPixel GetRLEValue(size_t i)
        { return m_Array[i].second; }

      size_t GetRLELength(size_t i)
        { return m_Array[i].first; }

      unsigned long GetUniqueID() const
        { return m_UniqueID; }

      Delta & operator = (const Delta &other)
      {
        m_Array = other.m_Array;
        m_CurrentLength = other.m_CurrentLength;
        m_LastValue = other.m_LastValue;
        return *this;
      }

    protected:
      typedef std::pair<size_t, TPixel> RLEPair;
      typedef std::vector<RLEPair> RLEArray;
      RLEArray m_Array;
      size_t m_CurrentLength;
      TPixel m_LastValue;

      // Each delta is assigned a unique ID at creation
      unsigned long m_UniqueID;
      static unsigned long m_UniqueIDCounter;
    };

  UndoDataManager(size_t nMinDeltas, size_t nMaxTotalSize);

  void AppendDelta(Delta *delta);
  void Clear();

  bool IsUndoPossible();
  Delta *GetDeltaForUndo();

  bool IsRedoPossible();
  Delta *GetDeltaForRedo();

  Delta *GetCumulativeDelta()
    { return m_CumulativeDelta; }

  void SetCumulativeDelta(Delta *);

  size_t GetNumberOfDeltas()
    { return m_DeltaList.size(); }

  /** 
   * A state descriptor. This descriptor is used to compare the
   * state of the undo queue between two time points. The idea is
   * to know whether an image has changed from the time it was 
   * saved or not. The state is basically the recording of all 
   * deltas from the starting point to the current position
   */
  typedef std::list<unsigned long> StateDescriptor;
  StateDescriptor GetState() const;

private:
  typedef std::list<Delta *> DList;
  typedef typename DList::iterator DIterator;
  typedef typename DList::const_iterator DConstIterator;
  DList m_DeltaList;
  DIterator m_Position;
  size_t m_TotalSize, m_MinDeltas, m_MaxTotalSize;

  Delta *m_CumulativeDelta;
};
