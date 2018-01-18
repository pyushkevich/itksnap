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

template<typename TPixel> unsigned long UndoDelta<TPixel>::m_UniqueIDCounter = 0;

template<typename TPixel>
UndoDelta<TPixel>
::UndoDelta()
{
  m_CurrentLength = 0;
  m_UniqueID = m_UniqueIDCounter++;
}

template<typename TPixel>
void
UndoDelta<TPixel>
::Encode(const TPixel &value)
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

template<typename TPixel>
void
UndoDelta<TPixel>
::FinishEncoding()
{
  if(m_CurrentLength > 0)
    m_Array.push_back(std::make_pair(m_CurrentLength, m_LastValue));
}

template<typename TPixel>
UndoDelta<TPixel> &
UndoDelta<TPixel>
::operator = (const UndoDelta<TPixel> &other)
{
  m_Array = other.m_Array;
  m_CurrentLength = other.m_CurrentLength;
  m_LastValue = other.m_LastValue;
  m_Region = other.m_Region;
  return *this;
}


template<typename TPixel>
UndoDataManager<TPixel>
::UndoDataManager(size_t nMinCommits, size_t nMaxTotalSize)
{
  this->m_MinCommits = nMinCommits;
  this->m_MaxTotalSize = nMaxTotalSize;
  this->m_TotalSize = 0;
  m_Position = m_CommitList.begin();
}

template<typename TPixel>
void
UndoDataManager<TPixel>
::Clear()
{
  // Delete all the commits
  m_Position = m_CommitList.begin();
  while(m_Position != m_CommitList.end())
    {
    // Deallocate all the deltas in this commit
    m_Position->DeleteDeltas();
    m_Position = m_CommitList.erase(m_Position);
    }
  m_TotalSize = 0;

  // Clear the staging list
  m_StagingList.clear();
}

template<typename TPixel>
void
UndoDataManager<TPixel>
::AddDeltaToStaging(Delta *delta)
{
  // Just add the delta to the staging list
  m_StagingList.push_back(delta);
}

template<typename TPixel>
int
UndoDataManager<TPixel>
::CommitStaging(const char *text)
{
  // If we are not currently pointing past the end of the delta
  // list, we should prune all the deltas from the current point
  // to the end. So that's the loop that we do
  while(m_Position != m_CommitList.end())
    {
    m_TotalSize -= m_Position->GetNumberOfRLEs();
    m_Position->DeleteDeltas();
    m_Position = m_CommitList.erase(m_Position);
    }

  // Create a commit that we will be adding
  Commit new_commit(m_StagingList, text);

  // Empty the staging list
  m_StagingList.clear();

  // Get the number of RLEs being added
  size_t n_new_rles = new_commit.GetNumberOfRLEs();

  // If there are no new rles, the just bail out
  if(n_new_rles == 0)
    {
    new_commit.DeleteDeltas();
    return 0;
    }

  // Check whether we need to prune from the back to keep total size under control
  CIterator itHead = m_CommitList.begin();
  while(m_CommitList.size() > m_MinCommits && m_TotalSize + n_new_rles > m_MaxTotalSize)
    {
    m_TotalSize -= itHead->GetNumberOfRLEs();
    itHead->DeleteDeltas();
    itHead = m_CommitList.erase(itHead);
    }

  // Now we have a well pruned list of deltas, and we can append
  // the current delta to it;
  m_CommitList.push_back(new_commit);
  m_Position = m_CommitList.end();
  m_TotalSize += n_new_rles;

  // Return the number of RLEs
  return n_new_rles;
}

template<typename TPixel>
bool
UndoDataManager<TPixel>
::IsUndoPossible()
{
  return (m_CommitList.size() > 0 && m_Position != m_CommitList.begin());
}

template<typename TPixel>
const typename UndoDataManager<TPixel>::Commit &
UndoDataManager<TPixel>
::GetCommitForUndo()
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
  return (m_CommitList.size() > 0 && m_Position != m_CommitList.end());
}

template<typename TPixel>
const typename UndoDataManager<TPixel>::Commit &
UndoDataManager<TPixel>
::GetCommitForRedo()
{
  // Can't be at the beginning
  assert(IsRedoPossible());

  // Return the delta at the current position
  const Commit &commit = *m_Position;

  // Move the position one delta to the end
  m_Position++;

  // Return the current delta
  return commit;
}



template<typename TPixel>
UndoDataManager<TPixel>::Commit::Commit(const DList &list, const char *name)
{
  m_Deltas = list;
  m_Name = name;
}

template<typename TPixel>
void
UndoDataManager<TPixel>::Commit::DeleteDeltas()
{
  // Delete all the deltas
  for(DIterator dit = m_Deltas.begin(); dit != m_Deltas.end(); ++dit)
    {
    if(*dit)
      {
      delete *dit;
      *dit = NULL;
      }
    }
}




template<typename TPixel>
size_t
UndoDataManager<TPixel>::Commit::GetNumberOfRLEs() const
{
  size_t n = 0;
  for(DConstIterator dit = m_Deltas.begin(); dit != m_Deltas.end(); ++dit)
    {
    if(*dit)
      n += (*dit)->GetNumberOfRLEs();
    }
  return n;
}
