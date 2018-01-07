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
  m_UndoManager = new UndoManagerType(4, 200000);
}

LabelImageWrapper::~LabelImageWrapper()
{
  delete m_UndoManager;
}

void LabelImageWrapper::UpdateImagePointer(
    ImageType *image, ImageBaseType *refSpace, ITKTransformType *tran)
{
  Superclass::UpdateImagePointer(image, refSpace, tran);
  m_UndoManager->Clear();

  // Modified event on the image is rebroadcast as the WrapperImageChangeEvent
  Rebroadcaster::Rebroadcast(image, itk::ModifiedEvent(),
                             this, WrapperImageChangeEvent());
}

void LabelImageWrapper::StoreIntermediateUndoDelta(UndoManagerDelta *delta)
{
  m_UndoManager->AddDeltaToStaging(delta);
}

void LabelImageWrapper::StoreUndoPoint(const char *text, UndoManagerDelta *delta)
{
  // If there is a delta, add it to staging
  if(delta)
    m_UndoManager->AddDeltaToStaging(delta);

  // Commit the deltas
  m_UndoManager->CommitStaging(text);
}

void LabelImageWrapper::ClearUndoPoints()
{
  m_UndoManager->Clear();
}

bool LabelImageWrapper::IsUndoPossible()
{
  return m_UndoManager->IsUndoPossible();
}

void LabelImageWrapper::Undo()
{
  // Get the commit for the undo
  const UndoManagerType::Commit &commit = m_UndoManager->GetCommitForUndo();

  // The label image that will undergo undo
  typedef itk::ImageRegionIterator<ImageType> IteratorType;
  ImageType *imSeg = this->GetImage();

  // Iterate over all the deltas in reverse order
  UndoManagerType::DList::const_reverse_iterator dit = commit.GetDeltas().rbegin();
  for(; dit != commit.GetDeltas().rend(); ++dit)
    {
    // Apply the changes in the current delta
    UndoManagerType::Delta *delta = *dit;

    // Iterator for the relevant region in the label image
    IteratorType lit(imSeg, delta->GetRegion());

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
  imSeg->Modified();
}

bool LabelImageWrapper::IsRedoPossible()
{
  return m_UndoManager->IsRedoPossible();
}

void LabelImageWrapper::Redo()
{
  // Get the commit for the redo
  const UndoManagerType::Commit &commit = m_UndoManager->GetCommitForRedo();

  // The label image that will undergo redo
  typedef itk::ImageRegionIterator<ImageType> IteratorType;
  ImageType *imSeg = this->GetImage();

  // Iterate over all the deltas in reverse order
  UndoManagerType::DList::const_iterator dit = commit.GetDeltas().begin();
  for(; dit != commit.GetDeltas().end(); ++dit)
    {
    // Apply the changes in the current delta
    UndoManagerType::Delta *delta = *dit;

    // Iterator for the relevant region in the label image
    IteratorType lit(imSeg, delta->GetRegion());

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
  imSeg->Modified();
}

LabelImageWrapper::UndoManagerDelta *
LabelImageWrapper::CompressImage() const
{
  UndoManagerDelta *new_cumulative = new UndoManagerDelta();
  ImageType *seg = this->GetImage();

  itk::ImageRegionConstIterator<ImageType> it(seg, seg->GetLargestPossibleRegion());
  for (; !it.IsAtEnd(); ++it)
    new_cumulative->Encode(it.Get());

  new_cumulative->FinishEncoding();
  return new_cumulative;
}
