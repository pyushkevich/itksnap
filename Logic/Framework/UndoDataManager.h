/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: UndoDataManager.h,v $
  Language:  C++
  Date:      $Date: 2007/12/25 15:46:23 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
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
        std::cout << "Delta size: " << m_Array.size() << std::endl;
        }

      size_t GetNumberOfRLEs()
        { return m_Array.size(); }

      TPixel GetRLEValue(size_t i)
        { return m_Array[i].second; }

      size_t GetRLELength(size_t i)
        { return m_Array[i].first; }

    protected:
      typedef std::pair<size_t, TPixel> RLEPair;
      typedef std::vector<RLEPair> RLEArray;
      RLEArray m_Array;
      size_t m_CurrentLength;
      TPixel m_LastValue;
    };

  UndoDataManager(size_t nMinDeltas, size_t nMaxTotalSize);

  void AppendDelta(Delta *delta);
  void Clear();

  bool IsUndoPossible();
  Delta *GetDeltaForUndo();

  bool IsRedoPossible();
  Delta *GetDeltaForRedo();

  size_t GetNumberOfDeltas()
    { return m_DeltaList.size(); }

private:
  typedef std::list<Delta *> DList;
  typedef typename DList::iterator DIterator;
  DList m_DeltaList;
  DIterator m_Position;
  size_t m_TotalSize, m_MinDeltas, m_MaxTotalSize;
};
