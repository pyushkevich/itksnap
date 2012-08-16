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
  // Override the number of inputs to 2, so that we can treat parameters
  // as the second input, and thus respond to parameter changes
  this->SetNumberOfInputs(2);
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
  if(settings != GetParameters())
    {
    // Store the settings as the second input
    this->SetNthInput(1, settings);
    }
}

template<typename TInputImage,typename TOutputImage>
ThresholdSettings *
SmoothBinaryThresholdImageFilter<TInputImage,TOutputImage>
::GetParameters()
{
  return static_cast<ThresholdSettings*>(this->GetInputs()[1].GetPointer());
}

template<typename TInputImage,typename TOutputImage>
void
SmoothBinaryThresholdImageFilter<TInputImage,TOutputImage>
::SmoothBinaryThresholdImageFilter::GenerateData()
{
  // Update the functor with the current settings
  this->GetFunctor().SetParameters(GetParameters());

  // Execute the filter
  Superclass::GenerateData();
}

template<class TPixel>
void
SmoothBinaryThresholdFunctor<TPixel>
::SetParameters(ThresholdSettings *settings)
{
  // At least one threshold should be used
  assert(settings->IsLowerThresholdEnabled() ||
         settings->IsUpperThresholdEnabled());

  // Store the upper and lower bounds
  m_LowerThreshold = settings->GetLowerThreshold();
  m_UpperThreshold = settings->GetUpperThreshold();

  // Handle the bad case: everything is mapped to zero
  if(m_LowerThreshold >= m_UpperThreshold)
    {
    m_ScalingFactor = m_FactorUpper = m_FactorLower = m_Shift = 0.0;
    return;
    }

  // Compute the largest scaling for U-L such that the function is greater
  // than 1-eps
  float eps = pow((float)10,-(float)settings->GetSmoothness());
  float maxScaling = (m_UpperThreshold - m_LowerThreshold) / log((2-eps)/eps);

  // Set the factor by which the input is multiplied
  // m_ScalingFactor = settings.GetSmoothness();
  m_ScalingFactor = 1 / maxScaling;

  // Combine the usage and inversion flags to get the scaling factors
  m_FactorLower = settings->IsLowerThresholdEnabled() ? 1.0f : 0.0f;
  m_FactorUpper = settings->IsUpperThresholdEnabled() ? 1.0f : 0.0f;

  // Compute the shift
  m_Shift = 1.0f - (m_FactorLower + m_FactorUpper);
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



