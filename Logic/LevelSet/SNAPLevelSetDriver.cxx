/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: SNAPLevelSetDriver.cxx,v $
  Language:  C++
  Date:      $Date: 2010/06/28 18:45:08 $
  Version:   $Revision: 1.2 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#include "SNAPLevelSetDriver.h"
#include "IRISVectorTypesToITKConversion.h"

#include "itkCommand.h"
#include "itkNarrowBandLevelSetImageFilter.h"
#include "itkParallelSparseFieldLevelSetImageFilter.h"
#include "itkDenseFiniteDifferenceImageFilter.h"
#include "LevelSetExtensionFilter.h"

// Disable some windows debug length messages
#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#pragma warning ( disable : 4503 )
#endif


template<unsigned int VDimension>
SNAPLevelSetDriver<VDimension>
::SNAPLevelSetDriver(FloatImageType *init, FloatImageType *speed,
                     const SnakeParameters &sparms,
                     VectorImageType *externalAdvection)
{
  // Create the level set function
  m_LevelSetFunction = LevelSetFunctionType::New();
  // m_LevelSetFunction = FastSNAPLevelSetFunction::New();

  // Pass the speed image to the function
  m_LevelSetFunction->SetSpeedImage(speed);  

  // Set the external advection if any
  if(externalAdvection)
    m_LevelSetFunction->SetAdvectionField(externalAdvection);

  // Remember the input and output images for later initialization
  m_InitializationImage = init;

  // Pass the parameters to the level set function
  AssignParametersToPhi(sparms,true);

  // Create the filter
  DoCreateLevelSetFilter();
}

template<unsigned int VDimension>
void 
SNAPLevelSetDriver<VDimension>
::AssignParametersToPhi(const SnakeParameters &p, bool irisNotUsed(firstTime))
{
  // Set up the level set function

  // The sign of the advection term is flipped in our equation
  m_LevelSetFunction->SetAdvectionWeight( - p.GetAdvectionWeight());
  m_LevelSetFunction->SetAdvectionSpeedExponent(p.GetAdvectionSpeedExponent());

  // The curvature exponent for traditional/legacy reasons has a +1 value.
  m_LevelSetFunction->SetCurvatureSpeedExponent(p.GetCurvatureSpeedExponent()+1);  
  m_LevelSetFunction->SetCurvatureWeight(p.GetCurvatureWeight());
  
  m_LevelSetFunction->SetPropagationWeight(p.GetPropagationWeight());
  m_LevelSetFunction->SetPropagationSpeedExponent(p.GetPropagationSpeedExponent());  
  m_LevelSetFunction->SetLaplacianSmoothingWeight(p.GetLaplacianWeight());
  m_LevelSetFunction->SetLaplacianSmoothingSpeedExponent(p.GetLaplacianSpeedExponent());  
  
  // We only need to recompute the internal images if the exponents to those
  // images have changed
  m_LevelSetFunction->CalculateInternalImages();
  
  // Call the initialize method
  m_LevelSetFunction->Initialize(to_itkSize(Vector3i(1)));

  // Set the time step
  m_LevelSetFunction->SetTimeStepFactor(
    p.GetAutomaticTimeStep() ? 1.0 : p.GetTimeStepFactor());

  // Remember the parameters
  m_Parameters = p;
}

template<unsigned int VDimension>
void
SNAPLevelSetDriver<VDimension>
::DoCreateLevelSetFilter()
{
  // In this method we have the flexibility to create a level set filter
  // of any ITK solver type.  This way, we can plug in different solvers:
  // NarrowBand, ParallelSparseField, even Dense.  
  if(m_Parameters.GetSolver() == SnakeParameters::PARALLEL_SPARSE_FIELD_SOLVER)
    {
    // Define an extension to the appropriate filter class
    typedef ParallelSparseFieldLevelSetImageFilter<
      FloatImageType,FloatImageType> LevelSetFilterType;
    typedef LevelSetExtensionFilter<LevelSetFilterType> ExtensionFilterType;
      
    // Create a new extended filter
    ExtensionFilterType::Pointer filter = ExtensionFilterType::New();

    // Cast this specific filter down to the lowest common denominator that is
    // a filter
    m_LevelSetFilter = filter.GetPointer();

    // Cast the specific filter to a generic interface, so we can call the 
    // extended operations without knowing exactly what the filter is (this 
    // is the beauty of polymorphism!)
    m_ExtensionView = filter.GetPointer();
    
    // Perform the special configuration tasks on the filter
    filter->SetInput(m_InitializationImage);
    filter->SetNumberOfLayers(3);
    filter->SetIsoSurfaceValue(0.0f);
    filter->SetDifferenceFunction(m_LevelSetFunction);
    }
  else if(m_Parameters.GetSolver() == SnakeParameters::NARROW_BAND_SOLVER)
    {
    // Define an extension to the appropriate filter class
    typedef NarrowBandLevelSetImageFilter<
      FloatImageType,FloatImageType> LevelSetFilterType;
    typedef LevelSetExtensionFilter<LevelSetFilterType> ExtensionFilterType;
    
    // Create a new extended filter
    ExtensionFilterType::Pointer filter = ExtensionFilterType::New();

    // Cast this specific filter down to the lowest common denominator that is
    // a filter
    m_LevelSetFilter = filter.GetPointer();

    // Cast the specific filter to a generic interface, so we can call the 
    // extended operations without knowing exactly what the filter is (this 
    // is the beauty of polymorphism!)
    m_ExtensionView = filter.GetPointer();
    
    // Perform the special configuration tasks on the filter
    filter->SetSegmentationFunction(m_LevelSetFunction);
    filter->SetInput(m_InitializationImage);
    filter->SetNarrowBandTotalRadius(5);
    filter->SetNarrowBandInnerRadius(3);
    filter->SetFeatureImage(m_LevelSetFunction->GetSpeedImage());  
    }
  else if(m_Parameters.GetSolver() == SnakeParameters::DENSE_SOLVER)
    {
    // Define an extension to the appropriate filter class
    typedef DenseFiniteDifferenceImageFilter<
      FloatImageType,FloatImageType> LevelSetFilterType;
    typedef LevelSetExtensionFilter<LevelSetFilterType> ExtensionFilterType;
    
    // Create a new extended filter
    ExtensionFilterType::Pointer filter = ExtensionFilterType::New();

    // Cast this specific filter down to the lowest common denominator that is
    // a filter
    m_LevelSetFilter = filter.GetPointer();

    // Cast the specific filter to a generic interface, so we can call the 
    // extended operations without knowing exactly what the filter is (this 
    // is the beauty of polymorphism!)
    m_ExtensionView = filter.GetPointer();
    
    // Perform the special configuration tasks on the filter
    filter->SetInput(m_InitializationImage);
    filter->SetDifferenceFunction(m_LevelSetFunction);
    }
}

template<unsigned int VDimension>
void
SNAPLevelSetDriver<VDimension>
::RequestRestart()
{ 
  // Makes no sense to call this if not in update cycle
  assert(this->IsInUpdateLoop());

  // Request a stop in the update
  m_ExtensionView->RequestStop();

  // Tell the method that called Update that it needs to call DoRestart()
  m_CommandAfterUpdate = SelfCommandType::New();
  m_CommandAfterUpdate->SetCallbackFunction(this,&SNAPLevelSetDriver::DoRestart);
}

template<unsigned int VDimension>
void
SNAPLevelSetDriver<VDimension>
::DoRestart()
{
  // To be on the safe side, just create a new filter (alternative is commented
  // out below, but seems to be unstable)
  DoCreateLevelSetFilter();

  // Update the image currently in the filter
  // m_LevelSetFilter->SetInput(m_InitializationImage);

  // Reset the filter, so the next time it is Updated, it will run again, even
  // if the input image has not changed
  // m_LevelSetFilter->Modified();
}

template<unsigned int VDimension>
void 
SNAPLevelSetDriver<VDimension>
::BeginUpdate(Command *pauseCallback)
{
  // This call may not nest
  assert(!this->IsInUpdateLoop());

  // This call loops until the filter returns from an update with no consequent
  // request specified
  while(true)
    {
    // Tell the filter how many iterations to perform
    m_ExtensionView->SetIterationsUntilPause(0);

    // Tell the filter to call back to the command passed in here.  If the 
    // command is NULL, the filter will return after executing the iterations
    m_ExtensionView->SetPauseCommand(pauseCallback);

    // Clear the post-update command
    m_CommandAfterUpdate = NULL;
    
    // Run the filter (at this point, we're stuck in this method, and rely on
    // the callback function to handle user interaction)
    m_LevelSetFilter->UpdateLargestPossibleRegion();

    // The control returns here after the filter finishes updating.  
    if(m_CommandAfterUpdate)
      {          
      // The callback may have set the post-update command pointer, asking us to 
      // execute another action before returning control to the parent
      m_CommandAfterUpdate->Execute(m_LevelSetFilter,AnyEvent());
      }      
    else
      {
      // There was no subsequent command requested, hence there was no  
      // reason for aborting Update() other that the user wants us to quit and
      // return control
      break;
      }      
    }
}

template<unsigned int VDimension>
bool
SNAPLevelSetDriver<VDimension>
::IsInUpdateLoop()
{
  return ((m_ExtensionView != NULL) && m_ExtensionView->IsUpdating());
}

template<unsigned int VDimension>
void 
SNAPLevelSetDriver<VDimension>
::RequestEndUpdate()
{
  // Makes no sense to call this if not in update cycle
  assert(this->IsInUpdateLoop());

  // Tell the filter it has to stop
  m_ExtensionView->RequestStop();
}


template<unsigned int VDimension>
void 
SNAPLevelSetDriver<VDimension>
::RequestIterations(int nIterations)
{
  // This method should only be called once the filter is updating, from the
  // pause callback
  assert(this->IsInUpdateLoop());
    
  // Since the filter is already running, so Run is being called from the 
  // pause callback.  In this case, we just tell the filter to run for more 
  // iterations once the pause callback returns
  m_ExtensionView->SetIterationsUntilPause(nIterations);
}

template<unsigned int VDimension>
typename SNAPLevelSetDriver<VDimension>::FloatImageType * 
SNAPLevelSetDriver<VDimension>
::GetCurrentState()
{
  // Fix the spacing of the level set filter's output
  m_LevelSetFilter->GetOutput()->SetSpacing(m_InitializationImage->GetSpacing());

  // Return the filter's output
  return m_LevelSetFilter->GetOutput();
}

template<unsigned int VDimension>
void 
SNAPLevelSetDriver<VDimension>
::CleanUp()
{
  // This method should not be called within the pause callback, or else
  // we would trash memory
  assert(!this->IsInUpdateLoop());

  // Basically, the filter is finished, and we can finally return 
  // from running the filter.  Let's clear the level set and the 
  // function to free memory
  m_LevelSetFilter = NULL;
  m_LevelSetFunction = NULL;
}

template<unsigned int VDimension>
void
SNAPLevelSetDriver<VDimension>
::SetSnakeParameters(const SnakeParameters &sparms)
{
  // Parameter setting can be destructive or passive.  If the solver has 
  // has changed, then it's destructive, otherwise it's passive
  bool destructive = sparms.GetSolver() != m_Parameters.GetSolver();

  // First of all, pass the parameters to the phi function, which may or
  // may not cause it to recompute it's images
  AssignParametersToPhi(sparms,false);

  if(destructive)
    {
    // We need to reinitialize the internal filter.  However, if the filter
    // is already running, all we can do is schedule that update
    if(this->IsInUpdateLoop())
      {
      // Tell the filter to stop when it regains control from the pause callback
      m_ExtensionView->RequestStop();

      // Schedule a subsequent call to create a new filter
      m_CommandAfterUpdate = SelfCommandType::New();
      m_CommandAfterUpdate->SetCallbackFunction(
        this,&SNAPLevelSetDriver::DoCreateLevelSetFilter);
      }
    else
      {
      // We are not in an update loop, but between updates, so just recreate the 
      // level set filter
      DoCreateLevelSetFilter();
      }
    }
}
