/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: UndoDataManager.txx,v $
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

template<typename TPixel>
unsigned long 
UndoDataManager<TPixel>::Delta::m_UniqueIDCounter = 0;

template<typename TPixel>
UndoDataManager<TPixel>
::UndoDataManager(size_t nMinDeltas, size_t nMaxTotalSize)
{
  this->m_MinDeltas = nMinDeltas;
  this->m_MaxTotalSize = nMaxTotalSize;
  this->m_TotalSize = 0;
  m_Position = m_DeltaList.begin();
  m_CumulativeDelta = NULL;
}

template<typename TPixel>
void
UndoDataManager<TPixel>
::Clear()
{
  // Delete all the deltas
  m_Position = m_DeltaList.begin();
  while(m_Position != m_DeltaList.end())
    {
    delete *m_Position;
    m_Position = m_DeltaList.erase(m_Position);
    }
  m_TotalSize = 0;
}

template<typename TPixel>
void
UndoDataManager<TPixel>
::AppendDelta(Delta *delta)
{
  // If we are not currently pointing past the end of the delta
  // list, we should prune all the deltas from the current point
  // to the end. So that's the loop that we do
  while(m_Position != m_DeltaList.end())
    {
    m_TotalSize -= (*m_Position)->GetNumberOfRLEs();
    delete *m_Position;
    m_Position = m_DeltaList.erase(m_Position);
    }

  // Check whether we need to prune from the back
  DIterator itHead = m_DeltaList.begin();
  while(m_DeltaList.size() > m_MinDeltas 
    && m_TotalSize + delta->GetNumberOfRLEs() > m_MaxTotalSize)
    {
    m_TotalSize -= (*itHead)->GetNumberOfRLEs();
    delete *itHead;
    itHead = m_DeltaList.erase(itHead);
    }

  // Now we have a well pruned list of deltas, and we can append
  // the current delta to it;
  m_DeltaList.push_back(delta);
  m_Position = m_DeltaList.end();
  m_TotalSize += delta->GetNumberOfRLEs();
}

template<typename TPixel>
bool
UndoDataManager<TPixel>
::IsUndoPossible()
{
  return (m_DeltaList.size() > 0 && m_Position != m_DeltaList.begin());
}

template<typename TPixel>
typename UndoDataManager<TPixel>::Delta *
UndoDataManager<TPixel>
::GetDeltaForUndo()
{
  // Can't be at the beginning
  assert(IsUndoPossible());

  // Move the position one delta to the beginning
  m_Position--;

  // Return the current delta
  return *m_Position;
}

template<typename TPixel>
bool
UndoDataManager<TPixel>
::IsRedoPossible()
{
  return (m_DeltaList.size() > 0 && m_Position != m_DeltaList.end());
}

template<typename TPixel>
typename UndoDataManager<TPixel>::Delta *
UndoDataManager<TPixel>
::GetDeltaForRedo()
{
  // Can't be at the beginning
  assert(IsRedoPossible());

  // Return the delta at the current position
  Delta *del = *m_Position;

  // Move the position one delta to the end
  m_Position++;

  // Return the current delta
  return del;
}

template<typename TPixel>
typename UndoDataManager<TPixel>::StateDescriptor
UndoDataManager<TPixel>
::GetState() const
{
  StateDescriptor sd;
  for(DConstIterator it = m_DeltaList.begin(); it != m_Position; ++it)
    {
    const Delta *delta = *it;
    sd.push_back(delta->GetUniqueID());
    }
  return sd;
}


template<typename TPixel>
void
UndoDataManager<TPixel>
::SetCumulativeDelta(Delta *delta)
{
  if(m_CumulativeDelta)
    delete m_CumulativeDelta;
  m_CumulativeDelta = delta;
}
