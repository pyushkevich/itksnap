/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SmoothBinaryThresholdImageFilter.txx,v $
  Language:  C++
  Date:      $Date: 2010/06/28 18:45:08 $
  Version:   $Revision: 1.3 $
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

template<typename TInputImage,typename TOutputImage>
SmoothBinaryThresholdImageFilter<TInputImage,TOutputImage>
::SmoothBinaryThresholdImageFilter()
  : Superclass()
{
  // Override the number of inputs to 4, so that we can respond to parameter
  // changes and image min/max changes automatically.
  this->SetNumberOfIndexedInputs(2);
  m_InputImageMaximum = 0;
  m_InputImageMinimum = 0;

  // Set the parameters (second input) to NULL
  m_Parameters = NULL;
}

template<typename TInputImage,typename TOutputImage>
void
SmoothBinaryThresholdImageFilter<TInputImage,TOutputImage>
::PrintSelf(std::ostream& os, itk::Indent indent) const
{
  Superclass::PrintSelf(os,indent);
}

template<typename TInputImage,typename TOutputImage>
void 
SmoothBinaryThresholdImageFilter<TInputImage,TOutputImage>
::SetParameters(ThresholdSettings *settings)
{
  if(settings != m_Parameters)
    {
    // Store the settings as the second input
	m_Parameters = settings;
    this->SetNthInput(1, m_Parameters);
    }
}

template<typename TInputImage,typename TOutputImage>
void
SmoothBinaryThresholdImageFilter<TInputImage,TOutputImage>
::GenerateData()
{
  // All inputs must be set!
  assert(m_Parameters);

  // Set up the functor
  this->GetFunctor().SetParameters(
        m_Parameters, m_InputImageMinimum, m_InputImageMaximum);

  // Execute the filter
  Superclass::GenerateData();
}

template<typename TInputImage,typename TOutputImage>
ThresholdSettings *
SmoothBinaryThresholdImageFilter<TInputImage,TOutputImage>
::GetParameters()
{
  return m_Parameters;
}

template<class TPixel>
void
SmoothBinaryThresholdFunctor<TPixel>
::SetParameters(ThresholdSettings *ts, double imin, double imax)
{
  // At least one threshold should be used
  assert(ts->IsLowerThresholdEnabled() || ts->IsUpperThresholdEnabled());

  // Bidirectional?
  bool bidir = ts->IsLowerThresholdEnabled() && ts->IsUpperThresholdEnabled();

  // Store the upper and lower bounds
  m_LowerThreshold = ts->GetLowerThreshold();
  m_UpperThreshold = ts->GetUpperThreshold();

  // Handle the bidirectional threshold invalid case
  if(bidir && m_LowerThreshold >= m_UpperThreshold)
    {
    m_ScalingFactor = m_FactorUpper = m_FactorLower = m_Shift = 0.0;
    return;
    }

  // Combine the usage and inversion flags to get the scaling factors
  m_FactorLower = ts->IsLowerThresholdEnabled() ? 1.0f : 0.0f;
  m_FactorUpper = ts->IsUpperThresholdEnabled() ? 1.0f : 0.0f;

  // Compute the shift
  m_Shift = 1.0f - (m_FactorLower + m_FactorUpper);

  // Compute the scaling factor (affecting smoothness) such that the supremum
  // of the threshold function on the image domain is equal to 1-10^-s, where s
  // is the smoothness factor
  double range = bidir
      ? m_UpperThreshold - m_LowerThreshold
      : (imax - imin) / 3;        // This is kind of arbitrary

  double eps = pow(10, -ts->GetSmoothness());
  m_ScalingFactor = static_cast<float>(log((2-eps)/eps) / range);
}

template<class TInput>
short
SmoothBinaryThresholdFunctor<TInput>
::operator ()(const TInput &x)
{
  // Cast the input to float
  float z = static_cast<float>(x);

  // Compute the left component
  float yLower = m_FactorLower * tanh((z-m_LowerThreshold) * m_ScalingFactor);

  // Compute the right component
  float yUpper = m_FactorUpper * tanh((m_UpperThreshold-z) * m_ScalingFactor);

  // Map to the range -1 ro 1
  float t = (yLower + yUpper + m_Shift);

  // Return the result (TODO: hack)
  return static_cast<short>(t * 0x7fff);
}

template<class TInput>
bool
SmoothBinaryThresholdFunctor<TInput>
::operator ==(const Self &z)
{
  return
      m_LowerThreshold == z.m_LowerThreshold &&
      m_UpperThreshold == z.m_UpperThreshold &&
      m_ScalingFactor == z.m_ScalingFactor &&
      m_FactorLower == z.m_FactorLower &&
      m_FactorUpper == z.m_FactorUpper &&
      m_Shift == z.m_Shift;
}

template<class TInput>
bool
SmoothBinaryThresholdFunctor<TInput>
::operator !=(const Self &z)
{
  return !(*this == z);
}



