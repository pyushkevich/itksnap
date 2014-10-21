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
#include "GMMClassifyImageFilter.h"
#include "GMMClassifyImageFilter.txx"
#include "RandomForestClassifyImageFilter.h"
#include "RandomForestClassifyImageFilter.txx"
#include "IRISApplication.h"
#include "UnsupervisedClustering.h"
#include "RFClassificationEngine.h"
#include "Rebroadcaster.h"

void
SmoothBinaryThresholdFilterConfigTraits
::AttachInputs(SNAPImageData *sid, FilterType *filter, int channel)
{
  // The work of attaching inputs to the filter is done in SetActiveScalarLayer,
  // so this method does nothing.
}

ScalarImageWrapperBase *
SmoothBinaryThresholdFilterConfigTraits
::GetDefaultScalarLayer(SNAPImageData *sid)
{
  return sid->GetMain()->GetDefaultScalarRepresentation();
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
::SetParameters(ParameterType *p, FilterType *filter, int channel)
{
  // Parameters are associated with the layer, so there is nothing to do here
}

void SmoothBinaryThresholdFilterConfigTraits::SetActiveScalarLayer(
    ScalarImageWrapperBase *layer, SmoothBinaryThresholdFilterConfigTraits::FilterType *filter, int channel)
{
  ScalarImageWrapperBase::CommonFormatImageType *image =
      layer->GetCommonFormatImage(
        static_cast<ScalarImageWrapperBase::ExportChannel>(channel));

  filter->SetInput(image);
  filter->SetInputImageMinimum(layer->GetImageMinAsDouble());
  filter->SetInputImageMaximum(layer->GetImageMaxAsDouble());

  // Check if we have threshold parameters already associated with this layer
  SmartPtr<ThresholdSettings> ts =
      dynamic_cast<ThresholdSettings *>(layer->GetUserData("ThresholdSettings"));

  if(!ts->GetInitialized())
    {
    // Associate threshold settings with this layer
    ts->InitializeToDefaultForImage(layer);
    }

  // Propagate modified events from the threshold settings object as events
  // from ImageWrapper. These events are further broadcast by GenericImageData
  // and IRISApplication, making it easy for GUI classes to respond to them
  Rebroadcaster::Rebroadcast(ts, itk::ModifiedEvent(),
                             layer, WrapperProcessingSettingsChangeEvent());

  // Pass the parameters to the filter
  filter->SetParameters(ts);
}

void
EdgePreprocessingFilterConfigTraits
::AttachInputs(SNAPImageData *sid, FilterType *filter, int channel)
{
  ScalarImageWrapperBase *scalar = sid->GetMain()->GetDefaultScalarRepresentation();
  ScalarImageWrapperBase::CommonFormatImageType *image =
      scalar->GetCommonFormatImage(
        static_cast<ScalarImageWrapperBase::ExportChannel>(channel));

  filter->SetInput(image);
  filter->SetInputImageMaximumGradientMagnitude(
        scalar->GetImageGradientMagnitudeUpperLimit());
}

void
EdgePreprocessingFilterConfigTraits
::DetachInputs(FilterType *filter)
{
  filter->SetInput(NULL);
}

void
EdgePreprocessingFilterConfigTraits
::SetParameters(ParameterType *p, FilterType *filter, int channel)
{
  filter->SetParameters(p);
}



void
GMMPreprocessingFilterConfigTraits
::AttachInputs(SNAPImageData *sid, FilterType *filter, int channel)
{
  // Iterate through all of the relevant layers
  for(LayerIterator it = sid->GetLayers(MAIN_ROLE | OVERLAY_ROLE);
      !it.IsAtEnd(); ++it)
    {
    if(it.GetLayerAsScalar())
      {
      AnatomicScalarImageWrapper *w = dynamic_cast<AnatomicScalarImageWrapper *>(it.GetLayer());
      filter->AddScalarImage(w->GetImage());
      }
    else if (it.GetLayerAsVector())
      {
      AnatomicImageWrapper *w = dynamic_cast<AnatomicImageWrapper *>(it.GetLayer());
      filter->AddVectorImage(w->GetImage());
      }
    }

  // Set the GMM input
  UnsupervisedClustering *uc = sid->GetParent()->GetClusteringEngine();
  assert(uc);
  filter->SetMixtureModel(uc->GetMixtureModel());
}

void
GMMPreprocessingFilterConfigTraits
::DetachInputs(FilterType *filter)
{
  while(filter->GetNumberOfValidRequiredInputs())
    filter->PopBackInput();

  filter->SetMixtureModel(NULL);
}

void
GMMPreprocessingFilterConfigTraits
::SetParameters(ParameterType *p, FilterType *filter, int channel)
{
  filter->SetMixtureModel(p);
}








void
RFPreprocessingFilterConfigTraits
::AttachInputs(SNAPImageData *sid, FilterType *filter, int channel)
{
  // Iterate through all of the relevant layers
  for(LayerIterator it = sid->GetLayers(MAIN_ROLE | OVERLAY_ROLE);
      !it.IsAtEnd(); ++it)
    {
    if(it.GetLayerAsScalar())
      {
      AnatomicScalarImageWrapper *w = dynamic_cast<AnatomicScalarImageWrapper *>(it.GetLayer());
      filter->AddScalarImage(w->GetImage());
      }
    else if (it.GetLayerAsVector())
      {
      AnatomicImageWrapper *w = dynamic_cast<AnatomicImageWrapper *>(it.GetLayer());
      filter->AddVectorImage(w->GetImage());
      }
    }

  // Set the classifier input
  RFClassificationEngine *rfe = sid->GetParent()->GetClassificationEngine();
  assert(rfe);
  filter->SetClassifier(rfe->GetClassifier());
}

void
RFPreprocessingFilterConfigTraits
::DetachInputs(FilterType *filter)
{
  while(filter->GetNumberOfValidRequiredInputs())
    filter->PopBackInput();

  filter->SetClassifier(NULL);
}

void
RFPreprocessingFilterConfigTraits
::SetParameters(ParameterType *p, FilterType *filter, int channel)
{
  filter->SetClassifier(p);
}

bool
RFPreprocessingFilterConfigTraits
::IsPreviewable(FilterType *filter[])
{
  return (filter[0]->GetClassifier() != NULL && filter[0]->GetClassifier()->IsValidClassifier());
}








// Instantiate preview wrappers
template class SlicePreviewFilterWrapper<SmoothBinaryThresholdFilterConfigTraits>;
template class SlicePreviewFilterWrapper<EdgePreprocessingFilterConfigTraits>;
template class SlicePreviewFilterWrapper<GMMPreprocessingFilterConfigTraits>;
template class SlicePreviewFilterWrapper<RFPreprocessingFilterConfigTraits>;

