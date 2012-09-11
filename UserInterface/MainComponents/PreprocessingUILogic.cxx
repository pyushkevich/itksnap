/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: PreprocessingUILogic.cxx,v $
  Language:  C++
  Date:      $Date: 2010/10/19 19:16:30 $
  Version:   $Revision: 1.7 $
  Copyright (c) 2007 Paul A. Yushkevich
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
// Borland compiler is very lazy so we need to instantiate the template
//  by hand 
#if defined(__BORLANDC__)
#include "SNAPBorlandDummyTypes.h"
#endif

#include "PreprocessingUILogic.h"

#include "GlobalState.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "IRISImageData.h"
#include "SNAPImageData.h"
#include "ThresholdSettings.h"
#include "UserInterfaceBase.h"

#include "itkImage.h"
#include "itkEventObject.h" 


void 
PreprocessingUILogic
::Register(UserInterfaceBase *parent)
{
  m_ParentUI = parent;
  m_Driver = parent->GetDriver();
  m_GlobalState = parent->GetDriver()->GetGlobalState();
}

void 
PreprocessingUILogic
::DisplayEdgeWindow()
{
  // Get the threshold parameters
  EdgePreprocessingSettings settings = 
    m_GlobalState->GetEdgePreprocessingSettings();

  // Set the slider values
  m_InEdgeScale->value(
    m_InEdgeScale->clamp(settings.GetGaussianBlurScale()));

  m_InEdgeKappa->value(
    m_InEdgeKappa->clamp(settings.GetRemappingSteepness()));

  m_InEdgeExponent->value(
    m_InEdgeExponent->clamp(settings.GetRemappingExponent()));

  // Position the window and show it
  m_ParentUI->CenterChildWindowInMainWindow(m_WinEdge);

  // Get a handle to the snap image data
  SNAPImageData *snapData = m_Driver->GetSNAPImageData();

  // Initialize the speed image if necessary, and assign it the correct
  // color map preset
  if(!snapData->IsSpeedLoaded())
    {
    snapData->InitializeSpeed();
    snapData->GetSpeed()->SetColorMap(
      SpeedColorMap::GetPresetColorMap(m_GlobalState->GetSpeedColorMap()));
    }
    
  // Set the intensity mapping mode for the speed image
  snapData->GetSpeed()->SetModeToEdgeSnake();

  // Apply the automatic preview preference
  m_InEdgePreview->value(
    m_GlobalState->GetShowPreprocessedEdgePreview() ? 1 : 0);

  OnEdgePreviewChange();

  // Set up the plot range, etc
  FunctionPlot2DSettings &plotSettings = 
    m_BoxEdgeFunctionPlot->GetPlotter().GetSettings();
  plotSettings.SetPlotRangeMin(Vector2f(0.0f));

  // Compute the plot to be displayed
  UpdateEdgePlot();

  // Show the window
  m_WinEdge->show();

  // Explicitly show the plotting box  
  m_BoxEdgeFunctionPlot->show();
}

void 
PreprocessingUILogic
::DisplayInOutWindow(void)
{
  // Get the threshold parameters
  ThresholdSettings settings = m_GlobalState->GetThresholdSettings();

  // Shorthands
  GreyTypeToNativeFunctor g2n =
    m_Driver->GetCurrentImageData()->GetGrey()->GetNativeMapping();
  float lower = g2n((GreyType)settings.GetLowerThreshold());
  float upper = g2n((GreyType)settings.GetUpperThreshold());

  // Set the ranges for the two thresholds.  These ranges do not require the
  // lower slider to be less than the upper slider, that will be corrected
  // dynamically as the user moves the sliders
  double iMin = m_Driver->GetCurrentImageData()->GetGrey()->GetImageMinNative();
  double iMax = m_Driver->GetCurrentImageData()->GetGrey()->GetImageMaxNative();

  m_InLowerThreshold->minimum(iMin);
  m_InLowerThresholdText->minimum(iMin);
  m_InLowerThreshold->maximum(iMax);
  m_InLowerThresholdText->maximum(iMax);

  m_InUpperThreshold->minimum(iMin);
  m_InUpperThresholdText->minimum(iMin);
  m_InUpperThreshold->maximum(iMax);
  m_InUpperThresholdText->maximum(iMax);

  //m_InThresholdSteepness->minimum(1);
  //m_InThresholdSteepness->maximum(iMax-iMin);
  m_InThresholdSteepness->minimum(1);
  m_InThresholdSteepness->maximum(10);

  // Make sure that the specified range is valid
  if(lower > upper)
    {
    lower = 0.67 * iMin + 0.33 * iMax;
    upper = 0.33 * iMin + 0.67 * iMax;
    }

  // Make sure the current values of the upper and lower threshold are 
  // within the bounds (Nathan Moon)
  SetLowerThresholdControlValue(m_InLowerThreshold->clamp(lower));
  SetUpperThresholdControlValue(m_InUpperThreshold->clamp(upper));

  m_InThresholdSteepness->value(
    m_InThresholdSteepness->clamp(settings.GetSmoothness()));

  // Set the radio buttons
  if(settings.IsLowerThresholdEnabled() && 
    settings.IsUpperThresholdEnabled())
    {
    m_RadioThresholdBoth->setonly();
    }
  else if(settings.IsLowerThresholdEnabled())
    {
    m_RadioThresholdBelow->setonly();
    }
  else
    {
    m_RadioThresholdAbove->setonly();
    }

  // Position the window and show it
  m_ParentUI->CenterChildWindowInMainWindow(m_WinInOut);

  // Get a handle to the snap image data
  SNAPImageData *snapData = m_Driver->GetSNAPImageData();

  // Initialize the speed image if necessary and assign it a color map
  if(!snapData->IsSpeedLoaded())
    {
    snapData->InitializeSpeed();
    snapData->GetSpeed()->SetColorMap(
      SpeedColorMap::GetPresetColorMap(m_GlobalState->GetSpeedColorMap()));
    }
  // Set the speed image to In/Out mode
  snapData->GetSpeed()->SetModeToInsideOutsideSnake();

  // Apply the automatic preview preference
  m_InThresholdPreview->value(
    m_GlobalState->GetShowPreprocessedInOutPreview() ? 1 : 0);
  OnThresholdPreviewChange();

  // Use a callback method to enable / disable sliders
  OnThresholdDirectionChange();

  // Apply the overlay settings (this is a personal preference, but I like 
  // to disable the color overlay when the window comes up initially)
  m_InThresholdOverlay->value(0);
  OnThresholdOverlayChange();

  // Set up the plot range, etc
  FunctionPlot2DSettings &plotSettings = 
    m_BoxThresholdFunctionPlot->GetPlotter().GetSettings();
  NativeToGreyTypeFunctor n2g(g2n);
  plotSettings.SetPlotRangeMin(Vector2f(n2g(iMin),-1.0f));
  plotSettings.SetPlotRangeMax(Vector2f(n2g(iMax),1.0f));

  // Compute the plot to be displayed
  UpdateThresholdPlot();

  // Show the window
  m_WinInOut->show();

  // Explicitly show the plotting box  
  m_BoxThresholdFunctionPlot->show();
}

void 
PreprocessingUILogic
::OnThresholdDirectionChange()
{
  // Enable and disable the state of the sliders based on the
  // current button settings
  if(m_RadioThresholdBoth->value())
    {
    m_InLowerThreshold->activate();
    m_InLowerThresholdText->activate();
    m_InUpperThreshold->activate();
    m_InUpperThresholdText->activate();
    }
  else if(m_RadioThresholdAbove->value())
    {
    m_InLowerThreshold->deactivate();
    m_InLowerThresholdText->deactivate();

    SetLowerThresholdControlValue(
      m_Driver->GetCurrentImageData()->GetGrey()->GetImageMinNative());
    
    m_InUpperThreshold->activate();
    m_InUpperThresholdText->activate();
    }
  else
    {
    m_InLowerThreshold->activate();
    m_InLowerThresholdText->activate();

    SetUpperThresholdControlValue(
      m_Driver->GetCurrentImageData()->GetGrey()->GetImageMaxNative());

    m_InUpperThreshold->deactivate();
    m_InUpperThresholdText->deactivate();
    }

  // The settings have changed, so call that method
  OnThresholdSettingsChange();
}

void PreprocessingUILogic
::SetUpperThresholdControlValue(double val)
{
  m_InUpperThreshold->value(val);
  m_InUpperThresholdText->value(val);
}

void PreprocessingUILogic
::SetLowerThresholdControlValue(double val)
{
  m_InLowerThreshold->value(val);
  m_InLowerThresholdText->value(val);
}

void 
PreprocessingUILogic
::OnThresholdLowerChange(double value)
{
  // Propagate the value to all controls
  SetLowerThresholdControlValue( value );

  // There may be a need to shift the upper bound
  if(m_InUpperThreshold->value() < m_InLowerThreshold->value())
    {
    SetUpperThresholdControlValue( m_InLowerThreshold->value() );
    }

  // Call the generic callback
  OnThresholdSettingsChange();
}

void 
PreprocessingUILogic
::OnThresholdUpperChange(double value)
{
  // Propagate the value to all controls
  SetUpperThresholdControlValue( value );

  // There may be a need to shift the lower bound
  if( m_InUpperThreshold->value() < m_InLowerThreshold->value())
    {
    SetLowerThresholdControlValue( m_InUpperThreshold->value() );
    }

  // Call the generic callback
  OnThresholdSettingsChange();
}

void 
PreprocessingUILogic
::OnEdgeSettingsChange()
{
  // Pass the current GUI settings to the filter
  EdgePreprocessingSettings settings;
  settings.SetGaussianBlurScale(m_InEdgeScale->value());
  settings.SetRemappingSteepness(m_InEdgeKappa->value());
  settings.SetRemappingExponent(m_InEdgeExponent->value());

  // Store the settings globally
  m_GlobalState->SetEdgePreprocessingSettings(settings);

  // Update the plotter
  UpdateEdgePlot();
  
  // Update display if in preview mode
  if(m_GlobalState->GetShowPreprocessedEdgePreview())
    {
    // Apply the settings to the preview filters
    m_EdgePreviewFilter[0]->SetEdgePreprocessingSettings(settings);
    m_EdgePreviewFilter[1]->SetEdgePreprocessingSettings(settings);
    m_EdgePreviewFilter[2]->SetEdgePreprocessingSettings(settings);

    // Repaint the slice windows
    m_ParentUI->RedrawWindows();
    }
}

void 
PreprocessingUILogic
::OnThresholdSettingsChange()
{
  // Pass the current GUI settings to the filter
  ThresholdSettings settings;
  GreyTypeToNativeFunctor g2n =
    m_Driver->GetCurrentImageData()->GetGrey()->GetNativeMapping();
  NativeToGreyTypeFunctor n2g(g2n);
  settings.SetLowerThreshold(n2g(m_InLowerThreshold->value()));
  settings.SetUpperThreshold(n2g(m_InUpperThreshold->value()));
  settings.SetSmoothness(m_InThresholdSteepness->value());
  settings.SetLowerThresholdEnabled(m_InLowerThreshold->active());
  settings.SetUpperThresholdEnabled(m_InUpperThreshold->active());  

  // Store the settings globally
  m_GlobalState->SetThresholdSettings(settings);

  // Compute the plot to be displayed
  UpdateThresholdPlot();
  
  // Apply the settings to the filter but only if we are in preview mode
  if(m_GlobalState->GetShowPreprocessedInOutPreview())
    {    
    m_InOutPreviewFilter[0]->SetThresholdSettings(settings);
    m_InOutPreviewFilter[1]->SetThresholdSettings(settings);
    m_InOutPreviewFilter[2]->SetThresholdSettings(settings);

    // Repaint the slice windows
    m_ParentUI->RedrawWindows();
    }  
}

void 
PreprocessingUILogic
::OnEdgePreviewChange(void)
{  
  bool preview = (m_InEdgePreview->value() == 1);

  // Store the value of the flag globally
  m_GlobalState->SetShowPreprocessedEdgePreview(preview);

  // Entering preview mode means that we can draw the speed image
  m_GlobalState->SetShowSpeed(preview);

  if(preview)
    {
    // Initialize each preview filter
    for(unsigned int i=0;i<3;i++)
      {
      // Make sure the preview filter is deallocated
      assert(!m_EdgePreviewFilter[i]);
  
      // Create the filter
      m_EdgePreviewFilter[i] = EdgeFilterType::New();

      // Give it an input
      m_EdgePreviewFilter[i]->SetInput(
        m_Driver->GetSNAPImageData()->GetGrey()->GetImage());
  
      // Pass the current settings to the filter
      m_EdgePreviewFilter[i]->SetEdgePreprocessingSettings(
        m_GlobalState->GetEdgePreprocessingSettings());
      }
        
    // Attach the preview filters to the corresponding slicers
    for(unsigned int j=0;j<3;j++)
      {
      // What is the image axis corresponding to the j-th slicer?
      unsigned int iSliceAxis = 
        m_Driver->GetSNAPImageData()->GetSpeed()->GetDisplaySliceImageAxis(j);

      // Connect the previewer to that slicer
      m_Driver->GetSNAPImageData()->GetSpeed()->SetSliceSourceForPreview(
        j,m_EdgePreviewFilter[iSliceAxis]->GetOutput());
      }

    // The speed preview is now valid
    m_GlobalState->SetSpeedPreviewValid(true);

    }
  else
    {
    // Clear the preview filters
    m_EdgePreviewFilter[0] = NULL;
    m_EdgePreviewFilter[1] = NULL;
    m_EdgePreviewFilter[2] = NULL;

    // Revert to the old speed image
    // TODO: We're reverting to the last APPLY.  The user may get confused.
    m_Driver->GetSNAPImageData()->GetSpeed()->RemoveSliceSourcesForPreview();

    // The speed preview is now invalid
    m_GlobalState->SetSpeedPreviewValid(false);
    }

  // Notify parent of the update
  m_ParentUI->OnPreprocessingPreviewStatusUpdate(preview);
}

void 
PreprocessingUILogic
::OnThresholdPreviewChange(void)
{
  bool preview = (m_InThresholdPreview->value() == 1);

  // Store the value of the flag globally
  m_GlobalState->SetShowPreprocessedInOutPreview(preview);

  // Entering preview mode means that we can draw the speed image
  m_GlobalState->SetShowSpeed(preview);

  if(preview)
    {
    // Initialize each preview filter
    for(unsigned int i=0;i<3;i++)
      {
      // Make sure the preview filter is deallocated
      assert(!m_InOutPreviewFilter[i]);
  
      // Create the filter
      m_InOutPreviewFilter[i] = InOutFilterType::New();

      // Give it an input
      m_InOutPreviewFilter[i]->SetInput(
        m_Driver->GetSNAPImageData()->GetGrey()->GetImage());
  
      // Pass the current settings to the filter
      m_InOutPreviewFilter[i]->SetThresholdSettings(
        m_GlobalState->GetThresholdSettings());
      }

    // Attach the preview filters to the corresponding slicers
    for(unsigned int j=0;j<3;j++)
      {
      // What is the image axis corresponding to the j-th slicer?
      unsigned int iSliceAxis = 
        m_Driver->GetSNAPImageData()->GetSpeed()->GetDisplaySliceImageAxis(j);

      // Connect the previewer to that slicer
      m_Driver->GetSNAPImageData()->GetSpeed()->SetSliceSourceForPreview(
        j,m_InOutPreviewFilter[iSliceAxis]->GetOutput());
      }

    // The speed preview is now valid
    m_GlobalState->SetSpeedPreviewValid(true);
    }
  else
    {
    // Clear the preview filters
    m_InOutPreviewFilter[0] = NULL;
    m_InOutPreviewFilter[1] = NULL;
    m_InOutPreviewFilter[2] = NULL;

    // Revert to the old speed image
    // TODO: We're reverting to the last APPLY.  The user may get confused.
    m_Driver->GetSNAPImageData()->GetSpeed()->RemoveSliceSourcesForPreview();

    // The speed preview is now invalid
    m_GlobalState->SetSpeedPreviewValid(false);
    }

  // Notify parent of the update
  m_ParentUI->OnPreprocessingPreviewStatusUpdate(preview);
}

void 
PreprocessingUILogic
::OnEdgeOk()
{
  // Apply the preprocessing on the whole image
  OnEdgeApply();

  // Run the same code as when the window is closed
  OnEdgeClose();
}

void 
PreprocessingUILogic
::OnThresholdOk(void)
{
  // Apply the preprocessing on the whole image
  OnThresholdApply();

  // Run the same code as when the window is closed
  OnThresholdClose();
}

void 
PreprocessingUILogic
::OnEdgeApply()
{
  // Create a callback object
  CommandPointer callback = CommandType::New();
  callback->SetCallbackFunction(this,&PreprocessingUILogic::OnEdgeProgress);
  
  // Use the SNAPImageData to perform preprocessing
  m_OutEdgeProgress->value(0);
  m_Driver->GetSNAPImageData()->DoEdgePreprocessing(
    m_GlobalState->GetEdgePreprocessingSettings(),callback);
  m_OutEdgeProgress->value(1);

  // The preprocessing image is valid
  m_GlobalState->SetSpeedValid(true);

  // Update the parent UI
  m_ParentUI->OnSpeedImageUpdate();
}

void 
PreprocessingUILogic
::OnThresholdApply()
{
  // Create a callback object
  CommandPointer callback = CommandType::New();
  callback->SetCallbackFunction(this,&PreprocessingUILogic::OnThresholdProgress);
  
  // Use the SNAPImageData to perform preprocessing
  m_Driver->GetSNAPImageData()->DoInOutPreprocessing(
    m_GlobalState->GetThresholdSettings(),callback);

  // The preprocessing image is valid
  m_GlobalState->SetSpeedValid(true);

  // Update the parent UI
  m_ParentUI->OnSpeedImageUpdate();
}

void 
PreprocessingUILogic
::OnThresholdOverlayChange(void)
{  
  if(m_InThresholdOverlay->value())
    {
    // These five lines of code pass the current drawing color to the overlay
    // system in the SpeedImageWrapper
    unsigned char rgbaOverlay[4];
    unsigned int label = m_GlobalState->GetDrawingColorLabel();
    m_Driver->GetColorLabelTable()->GetColorLabel(label).GetRGBAVector(rgbaOverlay);
    SpeedImageWrapper::OverlayPixelType colorOverlay(rgbaOverlay);
    m_Driver->GetSNAPImageData()->GetSpeed()->SetOverlayColor(colorOverlay);

    // Set the cutoff threshold to zero, i.e., positive speed values will be 
    // painted 
    m_Driver->GetSNAPImageData()->GetSpeed()->SetOverlayCutoff(0);

    // Set the appropriate flag
    m_GlobalState->SetSpeedViewZero(true);

    // Repaint
    m_ParentUI->RedrawWindows();
    }
  else 
    {
    // Clear the appropriate flag
    m_GlobalState->SetSpeedViewZero(false);
    }

  // Repaint the slice windows
  m_ParentUI->RedrawWindows();
}

void 
PreprocessingUILogic
::OnCloseCommon()
{
  // Revert to the old speed image
  // TODO: We're reverting to the last APPLY.  The user may get confused.
  m_Driver->GetSNAPImageData()->GetSpeed()->RemoveSliceSourcesForPreview();

  // The speed preview is now invalid
  m_GlobalState->SetSpeedPreviewValid(false);
  
  // Make sure that if the speed is not valid, then it is not visible
  if(!m_GlobalState->GetSpeedValid())
    {
    m_GlobalState->SetShowSpeed(false);
    m_ParentUI->RedrawWindows();
    }
    
  // Notify that we have been closed
  m_ParentUI->OnPreprocessClose();
}

void 
PreprocessingUILogic
::OnEdgeClose()
{
  // If in preview mode, disconnect and destroy the preview filters
  m_EdgePreviewFilter[0] = NULL;
  m_EdgePreviewFilter[1] = NULL;
  m_EdgePreviewFilter[2] = NULL;

  // Close the window
  m_WinEdge->hide();

  // Common closing tasks
  OnCloseCommon();
}

void 
PreprocessingUILogic
::OnThresholdClose(void)
{
  // Make sure we are no longer looking in grey/speed overlay mode
  m_GlobalState->SetSpeedViewZero(false);

  // If in preview mode, disconnect and destroy the preview filters
  m_InOutPreviewFilter[0] = NULL;
  m_InOutPreviewFilter[1] = NULL;
  m_InOutPreviewFilter[2] = NULL;

  // Close the window
  m_WinInOut->hide();
  
  // Common closing tasks
  OnCloseCommon();
}

void 
PreprocessingUILogic
::HidePreprocessingWindows()
{
  m_WinInOut->hide();
  m_WinEdge->hide();
}

void 
PreprocessingUILogic
::UpdateEdgePlot()
{
  // Create a functor object used in the filter
  EdgeFilterType::FunctorType functor;

  // Get the global settings
  EdgePreprocessingSettings settings = 
    m_GlobalState->GetEdgePreprocessingSettings();

  // Pass the settings to the functor
  functor.SetParameters(0.0f,1.0f,
    settings.GetRemappingExponent(),
    settings.GetRemappingSteepness());

  // Compute the function for a range of values
  const unsigned int nSamples = 200;
  float x[nSamples];
  float y[nSamples];

  for(unsigned int i=0;i<nSamples;i++) 
    {
    x[i] = i * 1.0f / (nSamples-1);
    y[i] = functor(x[i]);
    }

  // Pass the results to the plotter
  m_BoxEdgeFunctionPlot->GetPlotter().SetDataPoints(x,y,nSamples);

  // Redraw the box
  m_BoxEdgeFunctionPlot->redraw();
}

void 
PreprocessingUILogic
::UpdateThresholdPlot()
{
  // Create a functor object used in the filter
  SmoothBinaryThresholdFunctor<float,float> functor;

  // Get the global settings
  ThresholdSettings settings = 
    m_GlobalState->GetThresholdSettings();

  // We need to know the min/max of the image
  GreyTypeToNativeFunctor g2n =
    m_Driver->GetCurrentImageData()->GetGrey()->GetNativeMapping();
  NativeToGreyTypeFunctor n2g(g2n);
  float iMin = n2g(m_InLowerThreshold->minimum());
  float iMax = n2g(m_InUpperThreshold->maximum());

  // Pass the settings to the functor
  functor.SetParameters(iMin,iMax,settings);

  // Compute the function for a range of values
  const unsigned int nSamples = 200;
  float x[nSamples];
  float y[nSamples];

  for(unsigned int i=0;i<nSamples;i++) 
    {
    x[i] = iMin + i * (iMax-iMin) / (nSamples-1.0f);
    y[i] = functor(x[i]);
    }

  // Pass the results to the plotter
  m_BoxThresholdFunctionPlot->GetPlotter().SetDataPoints(x,y,nSamples);

  // Redraw the box
  m_BoxThresholdFunctionPlot->redraw();
}

#include <EdgePreprocessingImageFilter.h>

void 
PreprocessingUILogic
::OnEdgeProgress(itk::Object *object, const itk::EventObject &irisNotUsed(event))
{
  // Last progress value
  static double last_refresh = clock();
  static const double delta = CLOCKS_PER_SEC * 0.25;

  // Ignore event if nothing has happened
  if(last_refresh + delta < clock())
    {
    // Get the value of the progress
    float progress = dynamic_cast<itk::ProcessObject *>(object)->GetProgress();

    // Display the filter's progress
    m_OutEdgeProgress->value(progress);

    // Let the UI refresh
    Fl::check();

    // Reset the refresh counter
    last_refresh = clock();
    }
}

void 
PreprocessingUILogic
::OnThresholdProgress(itk::Object *object, const itk::EventObject &irisNotUsed(event))
{
  // Last progress value
  static double last_refresh = clock();
  static const double delta = CLOCKS_PER_SEC * 0.25;

  // Ignore event if nothing has happened
  if(last_refresh + delta < clock())
    {
    // Get the value of the progress
    float progress = dynamic_cast<itk::ProcessObject *>(object)->GetProgress();
    
    // Display the filter's progress
    m_OutThresholdProgress->value(progress);

    // Let the UI refresh
    Fl::check();

    // Reset the refresh counter
    last_refresh = clock();
    }
}

