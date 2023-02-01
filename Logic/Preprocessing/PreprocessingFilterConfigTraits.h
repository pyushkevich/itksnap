/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: IRISApplication.cxx,v $
  Language:  C++
  Date:      $Date: 2011/04/18 17:35:30 $
  Version:   $Revision: 1.37 $
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

#ifndef PREPROCESSINGFILTERCONFIGTRAITS_H
#define PREPROCESSINGFILTERCONFIGTRAITS_H

#include <SNAPImageData.h>
template <class TInput, class TOutput> class SmoothBinaryThresholdImageFilter;
template <class TInput, class TOutput> class EdgePreprocessingImageFilter;
template <class TInput, class TVectorInput, class TOutput> class GMMClassifyImageFilter;
template <class TInput, class TInputVector, class TOutput, class TLabel> class RandomForestClassifyImageFilter;

class ThresholdSettings;
class EdgePreprocessingSettings;
class GaussianMixtureModel;
template <class TPixel, class TLabel, int VDim> class RandomForestClassifier;


class AbstractFilterConfigTraits
{
public:
  /**
   * This method calls the layer's create cast to float pipeline method and stores
   * the resulting filter as UserData in the layer.
   */
  static ScalarImageWrapperBase::FloatImageType *
    CreateCastToFloatPipelineForLayer(ScalarImageWrapperBase *layer, int channel);

  /**
   * This method calls the layer's create cast to float pipeline method and stores
   * the resulting filter as UserData in the layer.
   */
  static VectorImageWrapperBase::FloatVectorImageType *
    CreateCastToFloatPipelineForLayer(VectorImageWrapperBase *layer, int channel);

  /**
   * This method deletes all the create cast to float pipelines created for all
   * the layers in the SNAPImageData class by this class
   */
  static void RemoveAllCastToFloatPipelines(SNAPImageData *sid);

};


class SmoothBinaryThresholdFilterConfigTraits : public AbstractFilterConfigTraits
{
public:

  typedef SNAPImageData                                            InputDataType;
  typedef ImageWrapperBase::FloatImageType                        InputImageType;
  typedef SNAPImageData::SpeedImageType                                SpeedType;
  typedef SpeedImageWrapper                                    OutputWrapperType;

  typedef SmoothBinaryThresholdImageFilter<InputImageType, SpeedType> FilterType;

  // The 'parameters' for this filter consist of just the scalar image layer to
  // which the filter should be applied. The actual parameters (thresholds) are
  // obtained from the selected layer's user data.
  typedef ThresholdSettings                                        ParameterType;

  static void AttachInputs(SNAPImageData *sid, FilterType *filter, int channel);
  static void DetachInputs(SNAPImageData *sid, FilterType *filter);
  static void SetParameters(ParameterType *p, FilterType *filter, int channel);
  static bool GetDefaultPreviewMode() { return true; }

  // This filter always has preview ready
  static bool IsPreviewable(FilterType *filter[]) { return true; }

  static ScalarImageWrapperBase* GetDefaultScalarLayer(SNAPImageData *sid);
  static void SetActiveScalarLayer(
      ScalarImageWrapperBase *layer, FilterType *filter, int channel);

protected:
  static std::string CasterPipelineKeyString(int channel);
};

class EdgePreprocessingFilterConfigTraits : public AbstractFilterConfigTraits
{
public:

  typedef SNAPImageData                                          InputDataType;
  typedef ImageWrapperBase::FloatImageType                      InputImageType;
  typedef SNAPImageData::SpeedImageType                              SpeedType;
  typedef SpeedImageWrapper                                  OutputWrapperType;

  typedef EdgePreprocessingImageFilter<InputImageType, SpeedType>   FilterType;
  typedef EdgePreprocessingSettings                              ParameterType;

  static void AttachInputs(SNAPImageData *sid, FilterType *filter, int channel);
  static void DetachInputs(SNAPImageData *sid, FilterType *filter);
  static void SetParameters(ParameterType *p, FilterType *filter, int channel);
  static bool GetDefaultPreviewMode() { return true; }

  // This filter always has preview ready
  static bool IsPreviewable(FilterType *filter[]) { return true; }

  static ScalarImageWrapperBase* GetDefaultScalarLayer(SNAPImageData *sid) { return NULL; }
  static void SetActiveScalarLayer(
      ScalarImageWrapperBase *layer, FilterType *filter, int channel) {}
};


class GMMPreprocessingFilterConfigTraits : public AbstractFilterConfigTraits
{
public:

  typedef SNAPImageData                                          InputDataType;

  typedef ImageWrapperBase::FloatImageType                FloatScalarImageType;
  typedef ImageWrapperBase::FloatVectorImageType          FloatVectorImageType;
  typedef SNAPImageData::SpeedImageType                              SpeedType;
  typedef SpeedImageWrapper                                  OutputWrapperType;

  typedef GMMClassifyImageFilter<
    FloatScalarImageType, FloatVectorImageType, SpeedType>          FilterType;

  typedef GaussianMixtureModel                                   ParameterType;

  static void AttachInputs(SNAPImageData *sid, FilterType *filter, int channel);
  static void DetachInputs(SNAPImageData *sid, FilterType *filter);
  static void SetParameters(ParameterType *p, FilterType *filter, int channel);
  static bool GetDefaultPreviewMode() { return true; }

  // This filter always has preview ready
  static bool IsPreviewable(FilterType *filter[]) { return true; }

  static ScalarImageWrapperBase* GetDefaultScalarLayer(SNAPImageData *sid) { return NULL; }
  static void SetActiveScalarLayer(
      ScalarImageWrapperBase *layer, FilterType *filter, int channel) {}
};


/** Traits class for random forest based preprocessing */
class RFPreprocessingFilterConfigTraits : public AbstractFilterConfigTraits
{
public:

  typedef SNAPImageData                                          InputDataType;

  typedef ImageWrapperBase::FloatImageType                FloatScalarImageType;
  typedef ImageWrapperBase::FloatVectorImageType          FloatVectorImageType;
  typedef SNAPImageData::SpeedImageType                              SpeedType;
  typedef SpeedImageWrapper                                  OutputWrapperType;

  typedef RandomForestClassifyImageFilter<
    FloatScalarImageType, FloatVectorImageType,
    SpeedType, LabelType>                                           FilterType;

  typedef RandomForestClassifier<float, LabelType, 3>            ParameterType;

  static void AttachInputs(SNAPImageData *sid, FilterType *filter, int channel);
  static void DetachInputs(SNAPImageData *sid, FilterType *filter);
  static void SetParameters(ParameterType *p, FilterType *filter, int channel);
  static bool GetDefaultPreviewMode() { return true; }

  // This filter always has preview ready
  static bool IsPreviewable(FilterType *filter[]);

  static ScalarImageWrapperBase* GetDefaultScalarLayer(SNAPImageData *sid) { return NULL; }
  static void SetActiveScalarLayer(
      ScalarImageWrapperBase *layer, FilterType *filter, int channel) {}
};




#endif // PREPROCESSINGFILTERCONFIGTRAITS_H
