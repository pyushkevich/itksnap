/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: LabelImageWrapper.cxx,v $
  Language:  C++
  Date:      $Date: 2018/01/05 $
  Version:   $Revision: 1 $
  Copyright (c) 2018 Paul A. Yushkevich

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
#include "LabelImageWrapper.h"
#include "UndoDataManager.h"
#include "Rebroadcaster.h"

LabelImageWrapper::LabelImageWrapper()
{
}

LabelImageWrapper::~LabelImageWrapper()
{
  for(auto p : m_TimePointUndoManagers)
    delete p;
}

void LabelImageWrapper::UpdateWrappedImages(
    Image4DType *image_4d,
    ImageBaseType *refSpace,
    ITKTransformType *tran)
{
  Superclass::UpdateWrappedImages(image_4d, refSpace, tran);

  // Clean up existing undo managers
  for(auto p : m_TimePointUndoManagers)
    delete p;

  // Set up new undo managers
  m_TimePointUndoManagers.resize(this->GetNumberOfTimePoints());
  for(auto &p : m_TimePointUndoManagers)
    p = new UndoManagerType(4, 200000);

  // Modified event on the image is rebroadcast as the WrapperImageChangeEvent
  Rebroadcaster::Rebroadcast(image_4d, itk::ModifiedEvent(), this, WrapperImageChangeEvent());

  // Modified event on each of the timepoints should also be rebroadcast
  for(auto &img : this->m_ImageTimePoints)
    Rebroadcaster::Rebroadcast(img, itk::ModifiedEvent(), this, WrapperImageChangeEvent());
}

void LabelImageWrapper::StoreIntermediateUndoDelta(UndoManagerDelta *delta)
{
  UndoManagerType *um = m_TimePointUndoManagers[m_TimePointIndex];
  um->AddDeltaToStaging(delta);
}

void LabelImageWrapper::StoreUndoPoint(const char *text, UndoManagerDelta *delta)
{
  UndoManagerType *um = m_TimePointUndoManagers[m_TimePointIndex];

  // If there is a delta, add it to staging
  if(delta)
    um->AddDeltaToStaging(delta);

  // Commit the deltas
  um->CommitStaging(text);
}

void LabelImageWrapper::ClearUndoPoints()
{
  UndoManagerType *um = m_TimePointUndoManagers[m_TimePointIndex];
  um->Clear();
}

void LabelImageWrapper::ClearUndoPointsForAllTimePoints()
{
  for(auto um : m_TimePointUndoManagers)
    um->Clear();
}

bool LabelImageWrapper::IsUndoPossible()
{
  UndoManagerType *um = m_TimePointUndoManagers[m_TimePointIndex];
  return um->IsUndoPossible();
}

void LabelImageWrapper::Undo()
{
  UndoManagerType *um = m_TimePointUndoManagers[m_TimePointIndex];

  // Get the commit for the undo
  const UndoManagerType::Commit &commit = um->GetCommitForUndo();

  // The label image that will undergo undo
  typedef itk::ImageRegionIterator<ImageType> IteratorType;

  // Iterate over all the deltas in reverse order
  UndoManagerType::DList::const_reverse_iterator dit = commit.GetDeltas().rbegin();
  for(; dit != commit.GetDeltas().rend(); ++dit)
    {
    // Apply the changes in the current delta
    UndoManagerType::Delta *delta = *dit;

    // Iterator for the relevant region in the label image
    IteratorType lit(m_Image, delta->GetRegion());

    // Iterate over the rles in the delta
    for(size_t i = 0; i < delta->GetNumberOfRLEs(); i++)
      {
      size_t n = delta->GetRLELength(i);
      LabelType d = delta->GetRLEValue(i);
      for(size_t j = 0; j < n; j++)
        {
        if(d != 0)
          lit.Set(lit.Get() - d);
        ++lit;
        }
      }
    }

  // Set modified flags
  this->PixelsModified();
}

bool LabelImageWrapper::IsRedoPossible()
{
  UndoManagerType *um = m_TimePointUndoManagers[m_TimePointIndex];
  return um->IsRedoPossible();
}

void LabelImageWrapper::Redo()
{
  UndoManagerType *um = m_TimePointUndoManagers[m_TimePointIndex];

  // Get the commit for the redo
  const UndoManagerType::Commit &commit = um->GetCommitForRedo();

  // The label image that will undergo redo
  typedef itk::ImageRegionIterator<ImageType> IteratorType;

  // Iterate over all the deltas in reverse order
  UndoManagerType::DList::const_iterator dit = commit.GetDeltas().begin();
  for(; dit != commit.GetDeltas().end(); ++dit)
    {
    // Apply the changes in the current delta
    UndoManagerType::Delta *delta = *dit;

    // Iterator for the relevant region in the label image
    IteratorType lit(m_Image, delta->GetRegion());

    // Iterate over the rles in the delta
    for(size_t i = 0; i < delta->GetNumberOfRLEs(); i++)
      {
      size_t n = delta->GetRLELength(i);
      LabelType d = delta->GetRLEValue(i);
      for(size_t j = 0; j < n; j++)
        {
        if(d != 0)
          lit.Set(lit.Get() + d);
        ++lit;
        }
      }
    }

  // Set modified flags
  this->PixelsModified();
}

const
LabelImageWrapper::UndoManagerType *
LabelImageWrapper
::GetUndoManager() const
{
  return m_TimePointUndoManagers[m_TimePointIndex];
}

LabelImageWrapper::UndoManagerDelta *
LabelImageWrapper::CompressImage() const
{
  UndoManagerDelta *new_cumulative = new UndoManagerDelta();

  itk::ImageRegionConstIterator<ImageType> it(m_Image, m_Image->GetLargestPossibleRegion());
  for (; !it.IsAtEnd(); ++it)
    new_cumulative->Encode(it.Get());

  new_cumulative->FinishEncoding();
  return new_cumulative;
}
