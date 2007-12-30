/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SNAPLevelSetStopAndGoFilter.txx,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:14 $
  Version:   $Revision: 1.2 $
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
template <class TInputImage, class TOutputImage>
void SNAPLevelSetStopAndGoFilter<TInputImage,TOutputImage>
::Start()
{
  // Allocate the output image
  typename TOutputImage::Pointer output = this->GetOutput();
  output->SetBufferedRegion(output->GetRequestedRegion());
  output->Allocate();

  // Copy the input image to the output image.  Algorithms will operate
  // directly on the output image and the update buffer.
  this->CopyInputToOutput();

  // Perform any other necessary pre-iteration initialization.
  this->Initialize();

  // Allocate the internal update buffer.  This takes place entirely within
  // the subclass, since this class cannot define an update buffer type.
  this->AllocateUpdateBuffer();

  // Reset the number of elapsed iterations
  this->SetElapsedIterations(0);

  // We have been initialized
  m_Started = true;  
}

template <class TInputImage, class TOutputImage>
void SNAPLevelSetStopAndGoFilter<TInputImage,TOutputImage>
::Run(unsigned int nIterations) 
{
  // Better be initialized
  assert(m_Started);

  for(unsigned int i=0;i<nIterations;i++)
    {
    this->InitializeIteration();

    TimeStepType dt = this->CalculateChange();

    this->ApplyUpdate(dt);

    this->SetElapsedIterations( this->GetElapsedIterations() + 1);
    }    
}

template <class TInputImage, class TOutputImage>
void SNAPLevelSetStopAndGoFilter<TInputImage,TOutputImage>
::GenerateData()
{
  if(!m_Started)
    Start();
}

template <class TInputImage, class TOutputImage>
SNAPLevelSetStopAndGoFilter<TInputImage,TOutputImage>
::SNAPLevelSetStopAndGoFilter()
{
  m_Started = false;
}

template <class TInputImage, class TOutputImage>
void SNAPLevelSetStopAndGoFilter<TInputImage,TOutputImage>
::PrintSelf(std::ostream &os, itk::Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "Running: " << m_Started << std::endl;
}

