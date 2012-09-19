/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SNAPImageData.cxx,v $
  Language:  C++
  Date:      $Date: 2011/04/18 17:35:30 $
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

=========================================================================*/

#include "PreprocessingFilterConfigTraits.h"
#include "SlicePreviewFilterWrapper.h"
#include "SlicePreviewFilterWrapper.txx"


void
SmoothBinaryThresholdFilterConfigTraits
::AttachInputs(SNAPImageData *sid, FilterType *filter)
{
  filter->SetInput(sid->GetGrey()->GetImage());
  filter->SetInputImageMinimum(sid->GetGrey()->GetImageMinAsDouble());
  filter->SetInputImageMaximum(sid->GetGrey()->GetImageMaxAsDouble());
}

void
SmoothBinaryThresholdFilterConfigTraits
::DetachInputs(FilterType *filter)
{
  filter->SetInput(NULL);
  filter->SetInputImageMinimum(0);
  filter->SetInputImageMaximum(0);
}

void
SmoothBinaryThresholdFilterConfigTraits
::SetParameters(ParameterType *p, FilterType *filter)
{
  filter->SetParameters(p);
}

void
EdgePreprocessingFilterConfigTraits
::AttachInputs(SNAPImageData *sid, FilterType *filter)
{
  filter->SetInput(sid->GetGrey()->GetImage());
  filter->SetInputImageMaximumGradientMagnitude(
        sid->GetGrey()->GetImageGradientMagnitudeUpperLimit());
}

void
EdgePreprocessingFilterConfigTraits
::DetachInputs(FilterType *filter)
{
  filter->SetInput(NULL);
}

void
EdgePreprocessingFilterConfigTraits
::SetParameters(ParameterType *p, FilterType *filter)
{
  filter->SetParameters(p);
}

// Instantiate preview wrappers
template class SlicePreviewFilterWrapper<SmoothBinaryThresholdFilterConfigTraits>;
template class SlicePreviewFilterWrapper<EdgePreprocessingFilterConfigTraits>;
