/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SnakeParametersUILogic.cxx,v $
  Language:  C++
  Date:      $Date: 2010/10/19 19:15:14 $
  Version:   $Revision: 1.10 $
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
// Borland compiler is very lazy so we need to instantiate the template
//  by hand 
#if defined(__BORLANDC__)
#include "SNAPBorlandDummyTypes.h"
#endif

#include "SnakeParametersUILogic.h"
#include "FL/fl_ask.H"

#include "itkEventObject.h" 
#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageRegionIterator.h"
#include "itkShiftScaleImageFilter.h"
#include "itkPNGImageIO.h"
#include "itkRGBPixel.h"
#include "GlobalState.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "IRISImageData.h"
#include "SNAPImageData.h"
#include "SNAPRegistryIO.h"
#include "SimpleFileDialogLogic.h"
#include "SnakeParametersPreviewPipeline.h"
#include "SystemInterface.h"
#include "ThresholdSettings.h"
#include "UserInterfaceBase.h"


void 
SnakeParametersUILogic
::OnAdvectionExponentChange(Fl_Valuator *input)
{
  int clamped = (int) input->clamp(input->value());
  m_Parameters.SetAdvectionSpeedExponent(clamped);
  this->OnParameterUpdate();
}

void 
SnakeParametersUILogic
::OnAdvectionWeightChange(Fl_Valuator *input)
{
  m_Parameters.SetAdvectionWeight(input->value());
  this->OnParameterUpdate();
}

void 
SnakeParametersUILogic
::OnCurvatureExponentChange(Fl_Valuator *input)
{
  int clamped = (int) input->clamp(input->value());
  m_Parameters.SetCurvatureSpeedExponent(clamped - 1);
  this->OnParameterUpdate();
}

void 
SnakeParametersUILogic
::OnCurvatureWeightChange(Fl_Valuator *input)
{
  m_Parameters.SetCurvatureWeight(input->value());
  this->OnParameterUpdate();
}

void 
SnakeParametersUILogic
::OnPropagationExponentChange(Fl_Valuator *input)
{
  int clamped = (int) input->clamp(input->value());
  m_Parameters.SetPropagationSpeedExponent(clamped);
  this->OnParameterUpdate();
}

void 
SnakeParametersUILogic
::OnPropagationWeightChange(Fl_Valuator *input)
{
  m_Parameters.SetPropagationWeight(input->value());
  this->OnParameterUpdate();
}

void 
SnakeParametersUILogic
::OnTimeStepChange(Fl_Valuator *input)
{
  m_Parameters.SetTimeStepFactor(input->value());
  this->OnParameterUpdate();
}

void 
SnakeParametersUILogic
::OnSmoothingWeightChange(Fl_Valuator *input)
{
  m_Parameters.SetLaplacianWeight(input->value());
  this->OnParameterUpdate();
}

void 
SnakeParametersUILogic
::OnLegacyClampChange(Fl_Check_Button *input)
{
  m_Parameters.SetClamp(input->value() > 0);
  this->OnParameterUpdate();
}

void 
SnakeParametersUILogic
::OnLegacyGroundChange(Fl_Valuator *input)
{ 
  m_Parameters.SetGround(input->value());
  this->OnParameterUpdate();
}

void 
SnakeParametersUILogic
::OnSolverChange()
{
  if(m_WarnOnSolverUpdate) 
    {
    int rc = 
      fl_choice("Changing the algorithm while level set evolution is running\n"
                "will cause the results to be lost!  Are you sure?",
                "Yes, change it","No",NULL);
    if(rc == 1) 
      {
      this->OnParameterUpdate();
      return;
      }      
    }

  SnakeParameters::SolverType solver;
  switch(m_InSolver->value()) 
    {
    case 0: solver = SnakeParameters::PARALLEL_SPARSE_FIELD_SOLVER; break;
    case 1: solver = SnakeParameters::NARROW_BAND_SOLVER; break;
    case 2: solver = SnakeParameters::DENSE_SOLVER; break;
    case 3: solver = SnakeParameters::LEGACY_SOLVER; break;
    default: solver = SnakeParameters::PARALLEL_SPARSE_FIELD_SOLVER;
    }

  if(solver != SnakeParameters::LEGACY_SOLVER)
    m_Parameters.SetAutomaticTimeStep(true);

  m_Parameters.SetSolver(solver);
  this->OnParameterUpdate();
}

void
SnakeParametersUILogic
::OnTimeStepAutoAction()
{
  m_Parameters.SetAutomaticTimeStep(m_BtnTimeStepAuto[0]->value() == 1);
  this->OnParameterUpdate();
}

void 
SnakeParametersUILogic
::OnAdvancedEquationAction()
{
  // Just call the parameters update method
  this->OnParameterUpdate();
}

void SnakeParametersUILogic
::OnParameterUpdate()
{
  // Propagate the values of the parameters to all the controls in 
  // this user interface

  // Activate or disactivate widgets based on the snake type
  if(m_Parameters.GetSnakeType() == SnakeParameters::EDGE_SNAKE)
    {
    m_GrpAdvectionEasy->show();
    m_GrpAdvectionMath->show();

    m_InAdvectionWeightMathText->show();
    m_InAdvectionExponentMathText->show();
    m_InCurvatureExponentMathText->show();
    m_InPropagationExponentMathText->show();

    m_InAdvectionWeightMathSlider->show();
    m_InAdvectionExponentMathSlider->show();
    m_InCurvatureExponentMathSlider->show();
    m_InPropagationExponentMathSlider->show();
    }
  else
    {
    m_GrpAdvectionEasy->hide();
    m_GrpAdvectionMath->hide();

    m_InAdvectionWeightMathText->hide();
    m_InAdvectionExponentMathText->hide();
    m_InCurvatureExponentMathText->hide();
    m_InPropagationExponentMathText->hide();

    m_InAdvectionWeightMathSlider->hide();
    m_InAdvectionExponentMathSlider->hide();
    m_InCurvatureExponentMathSlider->hide();
    m_InPropagationExponentMathSlider->hide();
    }

  // Curvature weight
  float value = m_Parameters.GetCurvatureWeight();
  m_InCurvatureWeightMathText->value(value);
  m_InCurvatureWeightMathSlider->value(value);
  m_InCurvatureWeightEasy->value(value);

  // Curvature exponent
  value = m_Parameters.GetCurvatureSpeedExponent() + 1;
  m_InCurvatureExponentMathText->value(value);
  m_InCurvatureExponentMathSlider->value(value);
  //m_InCurvatureExponentEasy->value(value);
  
  // Propagation weight 
  value = m_Parameters.GetPropagationWeight();
  m_InPropagationWeightMathText->value(value);
  m_InPropagationWeightMathSlider->value(value);
  //m_InPropagationWeightEasy->value(value);
  
  // Propagation exponent 
  value = m_Parameters.GetPropagationSpeedExponent();
  m_InPropagationExponentMathText->value(value);
  m_InPropagationExponentMathSlider->value(value);
  //m_InPropagationExponentEasy->value(value);
  
  // Advection weight 
  value = m_Parameters.GetAdvectionWeight();
  m_InAdvectionWeightMathText->value(value);
  m_InAdvectionWeightMathSlider->value(value);
  m_InAdvectionWeightEasy->value(value);
  
  // Advection exponent 
  value = m_Parameters.GetAdvectionSpeedExponent();
  m_InAdvectionExponentMathText->value(value);
  m_InAdvectionExponentMathSlider->value(value);
  //m_InAdvectionExponentEasy->value(value);

  // Experimental / normal equation display
  if(m_BtnAdvancedEquation->value() > 0)
    {
    // Show the right equation
    m_WizEquation->value(m_GrpEquationExperimental);

    // Enable all the controls
    m_InAdvectionExponentMathText->show();
    m_InAdvectionExponentMathSlider->show();
    m_InCurvatureExponentMathText->show();
    m_InCurvatureExponentMathSlider->show();
    m_InPropagationExponentMathText->show();    
    m_InPropagationExponentMathSlider->show();    
    m_GrpAdvectionEasy->show();
    }
  else
    {
    m_InAdvectionExponentMathText->hide();
    m_InAdvectionExponentMathSlider->hide();
    m_InCurvatureExponentMathText->hide();
    m_InCurvatureExponentMathSlider->hide();
    m_InPropagationExponentMathText->hide();    
    m_InPropagationExponentMathSlider->hide();    
    
    if(m_Parameters.GetSnakeType() == SnakeParameters::EDGE_SNAKE)
      {
      // Show the right equation
      m_WizEquation->value(m_GrpEquationEdge);

      m_GrpAdvectionEasy->show();
      m_GrpAdvectionMath->show();
      }
    else
      {
      // Show the right equation
      m_WizEquation->value(m_GrpEquationRegion);

      m_GrpAdvectionEasy->hide();
      m_GrpAdvectionMath->hide();
      }
    }

  // Update the display in the 2D preview boxes
  if(m_Parameters.GetSnakeType() == SnakeParameters::EDGE_SNAKE)
    m_PreviewPipeline->SetSpeedImage(m_ExampleImage[0]);
  else
    m_PreviewPipeline->SetSpeedImage(m_ExampleImage[1]);
  
  // Pass the parameters to the pipeline
  m_PreviewPipeline->SetSnakeParameters(m_Parameters);
    
  // Advanced page : solver
  if(m_Parameters.GetSolver() == SnakeParameters::LEGACY_SOLVER) 
    {
    // Show the right solver
    m_InSolver->value(2);

    // Show the options page
    m_WizSolverOptions->value(m_GrpLegacySolverOptions);

    // Fill the option page controls
    m_ChkLegacyClamp->value(m_Parameters.GetClamp() ? 1 : 0);
    m_InLegacyGround->value(m_Parameters.GetGround());

    // Disable the automatic timestep selection
    m_Parameters.SetAutomaticTimeStep(false);
    m_BtnTimeStepAuto[0]->deactivate();
    }
  else
    {
    if(m_Parameters.GetSolver() == SnakeParameters::NARROW_BAND_SOLVER)
      {
      // Show the right solver
      m_InSolver->value(1);

      // Show the options page
      m_WizSolverOptions->value(m_GrpNarrowSolverOptions);
      }
    else if(m_Parameters.GetSolver() == SnakeParameters::DENSE_SOLVER)
      {
      // Show the right solver
      m_InSolver->value(2);

      // Show the options page
      m_WizSolverOptions->value(m_GrpNarrowSolverOptions);
      }
    else
      {
      // Show the right solver
      m_InSolver->value(0);

      // Show the options page
      m_WizSolverOptions->value(m_GrpSparseSolverOptions);
      }

    // Enable the automatic timestep selection
    m_BtnTimeStepAuto[0]->activate();
    }

  // Advanced page : time step
  if(m_Parameters.GetAutomaticTimeStep())
    {
    m_BtnTimeStepAuto[0]->setonly();
    m_InTimeStep->deactivate();
    m_Parameters.SetLaplacianWeight(0);
    m_InSmoothingWeight->deactivate();
    }
  else
    {
    m_BtnTimeStepAuto[1]->setonly();
    m_InTimeStep->activate();
    m_InSmoothingWeight->activate();
    }

  m_InTimeStep->value(m_Parameters.GetTimeStepFactor());
  m_InSmoothingWeight->value(m_Parameters.GetLaplacianWeight());

  // Update the parameter display windows
  RedrawAllBoxes();
}

void 
SnakeParametersUILogic
::RedrawAllBoxes()
{
  m_BoxPreview[0]->redraw();
  m_BoxPreview[1]->redraw();
  m_BoxPreview[2]->redraw();
  m_BoxPreview[3]->redraw();
}



void 
SnakeParametersUILogic
::OnHelpAction()
{

}

void SnakeParametersUILogic
::CloseWindow()
{
  // Close the window
  m_Window->hide();
}

void SnakeParametersUILogic
::OnOkAction()
{
  m_UserAccepted = true;
  CloseWindow();
}

void SnakeParametersUILogic
::OnCloseAction()
{
  m_UserAccepted = false;
  CloseWindow();
}

void SnakeParametersUILogic
::OnSaveParametersAction()
{
  // Show the save dialog using the correct history
  m_IODialog->SetTitle("Save Snake Parameters");
  m_IODialog->DisplaySaveDialog(
    m_SystemInterface->GetHistory("SnakeParameters"),NULL);
}

void SnakeParametersUILogic
::SaveParametersCallback()
{
  // Get the selected file name
  const char *file = m_IODialog->GetFileName();

  try 
    {
    // Create a registry for the file
    Registry regParameters;

    // Use a registry IO object to put the parameters into a registry
    SNAPRegistryIO().WriteSnakeParameters(m_Parameters,regParameters);

    // Write the registry to file
    regParameters.WriteToFile(file,"# ITK-SNAP Snake Parameters File");
    
    // Add the filename to the history
    m_SystemInterface->UpdateHistory("SnakeParameters",file);
    }
  catch(...)
    {
    fl_alert("Unable to write to file %s!",file);  
    throw;
    }
}

void SnakeParametersUILogic
::OnLoadParametersAction()
{
  // Show the save dialog using the correct history
  m_IODialog->SetTitle("Load Snake Parameters");
  m_IODialog->DisplayLoadDialog(
    m_SystemInterface->GetHistory("SnakeParameters"),NULL);
}

void SnakeParametersUILogic
::LoadParametersCallback()
{
  // Get the selected file name
  const char *file = m_IODialog->GetFileName();
  SnakeParameters parameters;

  try 
    {
    // Read a registry from the indicated file
    Registry regParameters(file);

    // Read the parameters from the registry
    parameters = 
      SNAPRegistryIO().ReadSnakeParameters(regParameters,m_Parameters);
    }
  catch(...)
    {
    fl_alert("Unable to load parameters from file %s!",file);  
    throw;
    }

  // Make sure the parameters are of valid type
  if(parameters.GetSnakeType() != m_Parameters.GetSnakeType())
    {
    int rc = 0;
    if(m_Parameters.GetSnakeType() == SnakeParameters::EDGE_SNAKE)
      {
      rc = fl_choice(
        "Warning!  The snake evolution parameters in the file are for the\n"
        "REGION COMPETITION mode.  ITK-SNAP is currently in EDGE STOPPING mode.\n"
        "Do you wish to load the parameters anyway?",
        "Yes", "No", NULL);
      }
    else
      {
      rc = fl_choice(
        "Warning!  The snake evolution parameters in the file are for the\n"
        "EDGE STOPPING mode.  ITK-SNAP is currently in REGION COMPETITION mode.\n"
        "Do you wish to load the parameters anyway?",
        "Yes", "No", NULL);
      }

    // Show the message
    if(rc == 1) throw rc;

    // If region competition, drop the advection stuff
    if(m_Parameters.GetSnakeType() == SnakeParameters::REGION_SNAKE)
      {
      parameters.SetAdvectionSpeedExponent(0);
      parameters.SetAdvectionWeight(0);
      parameters.SetCurvatureSpeedExponent(-1);
      }

    // Keep the mode
    parameters.SetSnakeType(m_Parameters.GetSnakeType());
    }

  // Set the parameters
  SetParameters(parameters);

  // Add the filename to the history
  m_SystemInterface->UpdateHistory("SnakeParameters",file);
}

void SnakeParametersUILogic
::SetParameters(const SnakeParameters &parms)
{
  // Set the parameters
  m_Parameters = parms;

   // Update the user interface controls
  OnParameterUpdate();
}

void SnakeParametersUILogic
::Register(UserInterfaceBase *parent)
{
  // Get the parent's system object
  m_ParentUI = parent;
  m_SystemInterface = parent->GetSystemInterface();
  
  // Get the edge and region example image file names
  string fnImage[2];
  fnImage[0] = 
    m_SystemInterface->GetFileInRootDirectory("Images2D/EdgeForcesExample.png");
  fnImage[1] = 
    m_SystemInterface->GetFileInRootDirectory("Images2D/RegionForcesExample.png");

  // Typedefs
  typedef itk::ImageFileReader<ExampleImageType> ReaderType;
  typedef itk::ImageRegionIterator<ExampleImageType> IteratorType;
  typedef itk::ShiftScaleImageFilter<ExampleImageType, ExampleImageType> ScaleShiftType;

  // Initialize the pipeline
  m_PreviewPipeline = new SnakeParametersPreviewPipeline(
    m_ParentUI->GetDriver()->GetGlobalState());

  // Load each of these images
  static const float scale_factor[] = { 1.0f / 255.0f, -2.0f / 255.0f };
  static const float shift_factor[] = { 0.0f, -127.5f };

  for(unsigned int i = 0; i < 2; i++) 
    {
    try 
      {
      // Read the image in
      ReaderType::Pointer reader = ReaderType::New();
      reader->SetFileName(fnImage[i].c_str());

      ScaleShiftType::Pointer scaler = ScaleShiftType::New();
      scaler->SetScale(scale_factor[i]);
      scaler->SetShift(shift_factor[i]);
      scaler->SetInput(reader->GetOutput());

      scaler->Update();

      // Allocate the example image
      m_ExampleImage[i] = scaler->GetOutput();
      }
    catch(itk::ExceptionObject &exc)
      {
      // An exception occurred.  
      fl_alert("Unable to load image %s\n"
               "Exception %s\n"
               "Force illustration example will not be available.", 
               exc.GetDescription(), fnImage[i].c_str());

      // Initialize an image to zeros
      m_ExampleImage[i] = NULL;
      }
    }

  // Load the points from the registry
  std::vector<Vector2d> points;
  string fnPreset = m_SystemInterface->GetFileInRootDirectory(
      "Presets/SnakeParameterPreviewCurve.txt");
  try 
    {
    Registry regPoints(fnPreset.c_str());
    points = regPoints.Folder("Points").GetArray(Vector2d(0.0));
    }
  catch(...)
    {
    // An exception occurred.  
    fl_alert("Unable to load file %s\n"
             "Force illustration example will not be available.",
             fnPreset.c_str());
    }
  
  // Assign our previewer to the preview windows
  for(unsigned int i=0;i<4;i++)
    {
    m_BoxPreview[i]->SetParentUI(this);
    m_BoxPreview[i]->SetPipeline(m_PreviewPipeline);
    }
  
  // If there are some points in there, draw them
  if(points.size() >= 4)
    {
    // Set spline points, etc
    m_PreviewPipeline->SetControlPoints(points);

    // Set which forces to display
    m_BoxPreview[0]->SetForceToDisplay(SnakeParametersPreviewBox::PROPAGATION_FORCE);
    m_BoxPreview[1]->SetForceToDisplay(SnakeParametersPreviewBox::CURVATURE_FORCE);
    m_BoxPreview[2]->SetForceToDisplay(SnakeParametersPreviewBox::ADVECTION_FORCE);
    m_BoxPreview[3]->SetForceToDisplay(SnakeParametersPreviewBox::TOTAL_FORCE);
    }

  // Don't warn by default
  m_WarnOnSolverUpdate = false;
}

void 
SnakeParametersUILogic
::DisplayWindow()
{
  // Show everything
  m_Window->show();
  for(unsigned int j=0;j<4;j++) 
    {
    m_BoxPreview[j]->show();
    }

  // Update parameters
  OnParameterUpdate();
}

SnakeParametersUILogic
::SnakeParametersUILogic()
  : SnakeParametersUI()
{
  // Clear the pipeline
  m_PreviewPipeline = NULL;

  // Create the parameter IO dialog window
  m_IODialog = new SimpleFileDialogLogic;
  m_IODialog->MakeWindow();
  m_IODialog->SetFileBoxTitle("Snake Parameters File:");
  m_IODialog->SetPattern("Text files\t*.txt");
  m_IODialog->SetLoadCallback(
    this,&SnakeParametersUILogic::LoadParametersCallback);
  m_IODialog->SetSaveCallback(
    this,&SnakeParametersUILogic::SaveParametersCallback);
}

SnakeParametersUILogic
::~SnakeParametersUILogic()
{
  if(m_PreviewPipeline)
    delete m_PreviewPipeline;
  delete m_IODialog;
}

void 
SnakeParametersUILogic
::ShowHelp(const char *link)
{
  m_ParentUI->ShowHTMLPage(link);
}

void
SnakeParametersUILogic
::OnAnimateAction()
{
  // Remove a timeout callback if it exists
  Fl::remove_timeout(SnakeParametersUILogic::OnTimerCallback, this);
  m_PreviewPipeline->SetDemoLoopRunning(false);

  // If the button value is one, add the timeout
  if(m_BtnAnimate->value())
    {
    Fl::add_timeout(0.2, SnakeParametersUILogic::OnTimerCallback, this);
    m_PreviewPipeline->SetDemoLoopRunning(true);
    }
  else
    {
    RedrawAllBoxes();
    }
}

void 
SnakeParametersUILogic
::OnTimerCallback(void *cbdata)
{
  // Get the object that this refers to
  SnakeParametersUILogic *self = 
    reinterpret_cast<SnakeParametersUILogic *>(cbdata);

  // If the window is not visible, there is nothing to do
  if(!self->m_Window->visible())
    {
    self->m_PreviewPipeline->SetDemoLoopRunning(false);
    self->m_BtnAnimate->value(0);
    }
  else
    {
    // Call the pipeline's animation method
    self->m_PreviewPipeline->AnimationCallback();

    // Redraw all the windows
    self->RedrawAllBoxes();

    // Schedule another run
    Fl::repeat_timeout(0.05, SnakeParametersUILogic::OnTimerCallback, self);
    }
}

void 
SnakeParametersUILogic
::OnSpeedColorMapUpdate()
{
  if(m_Window->visible())
    RedrawAllBoxes();
}
