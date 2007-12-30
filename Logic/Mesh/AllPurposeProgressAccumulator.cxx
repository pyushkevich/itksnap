/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: AllPurposeProgressAccumulator.cxx,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:15 $
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
#include "AllPurposeProgressAccumulator.h"
#include "vtkCallbackCommand.h" 
#include "itkCommand.h"
#include "itkEventObject.h"
#include <cassert>

AllPurposeProgressAccumulator
::AllPurposeProgressAccumulator()
{ 
  SetProgress(0.0);
  m_Started = m_Ended = false;
}

void AllPurposeProgressAccumulator::ResetProgress() 
{ 
  // Set the progress and state of every source
  for(SourceIter it = m_Source.begin(); it != m_Source.end(); ++it)
    {
    // Reset the status of each run
    for(unsigned int i = 0; i < it->second.Runs.size(); i++)
      {
      it->second.Runs[i].Progress = 0.0;
      it->second.Runs[i].Started = false;
      it->second.Runs[i].Ended = false;
      }

    // Set the run counter to 0
    it->second.RunId = 0;

    DebugPrint(it->first, "RESET");
    }

  // Set the global progress and state
  SetProgress(0.0f);
  m_Started = m_Ended = false;
}

void
AllPurposeProgressAccumulator
::CallbackStart(void *source)
{
  // Make sure source is registered
  assert(m_Source.find(source) != m_Source.end());

  DebugPrint(source, "START");

  // Check whether this is a valid start event. To be valid, there must be at
  // least one run associated with source that is in state NoEvent and no run is
  // state StartEvent
  ProgressData &pd = m_Source[source];
  
  // Make sure that there are more runs left for this source
  assert( pd.RunId < pd.Runs.size() );
  RunData &run = pd.Runs[pd.RunId];
  
  // Never should have a start on an event that's ended in theory, but it
  // happens at least in VTK
  if(run.Ended) return;

  // It is possible it seems to receive multiple start events...
  if(run.Started) return;
  
  // Register the start of a new run
  run.Started = true;

  // Check if this is the first run overall
  bool firstRun = !m_Started;
  
  // Compute total progress and overall event state
  ComputeTotalProgressAndState();
  
  // Invoke overall event
  if(firstRun)
    InvokeEvent(itk::StartEvent());
  else
    InvokeEvent(itk::ProgressEvent());
}

void
AllPurposeProgressAccumulator
::CallbackProgress(void *source, double progress)
{
  DebugPrint(source, "PROGRESS", progress);

  // Make sure source is registered
  assert(m_Source.find(source) != m_Source.end());

  // Get the progress data
  ProgressData &pd = m_Source[source];
  
  // Make sure that there are more runs left for this source
  assert( pd.RunId < pd.Runs.size() );
  RunData &run = pd.Runs[pd.RunId];
  
  // Make sure that the current run is in the correct state
  assert(run.Started);

  // If the run has finished, and event is fired, ignore it
  // if(run.Ended) return;

  // Set the progress for this run, and recompute the total progress
  run.Progress = progress;
  
  // Compute total progress and overall event state
  ComputeTotalProgressAndState();
  
  // Invoke overall event
  InvokeEvent(itk::ProgressEvent());
}

void
AllPurposeProgressAccumulator
::CallbackEnd(void *source, double progress)
{
  // Make sure source is registered
  assert(m_Source.find(source) != m_Source.end());

  DebugPrint(source, "END", progress);

  // Get the progress data
  ProgressData &pd = m_Source[source];
  
  // Make sure that there are more runs left for this source
  assert( pd.RunId < pd.Runs.size() );
  RunData &run = pd.Runs[pd.RunId];
  
  // Make sure that the current run is in the correct state
  assert(run.Started);

  // It is possible to get two end events in a row...
  // if(run.Ended) return;

  // It is also possible to get an end event before we finish
  if(progress < 1.0) return;

  // Set the progress for this run, and recompute the total progress
  run.Ended = true;
  run.Progress = 1.0;

  // Compute total progress and overall event state
  ComputeTotalProgressAndState();
  
  // Invoke overall event
  if(m_Ended)
    InvokeEvent(itk::EndEvent());
  else
    InvokeEvent(itk::ProgressEvent());
}

void
AllPurposeProgressAccumulator
::StartNextRun(void *source)
{
  // Make sure source is registered
  assert(m_Source.find(source) != m_Source.end());

  // Get the progress data
  ProgressData &pd = m_Source[source];
  
  // Make sure that there are more runs left for this source
  assert( pd.RunId < pd.Runs.size() );

  // Move to the next run
  pd.RunId++;
}

void 
AllPurposeProgressAccumulator
::CallbackVTK(
  vtkObject *source, unsigned long eventId, void *clientdata, void *callData)
{
  // Figure out the pointers
  vtkAlgorithmClass *alg = dynamic_cast<vtkAlgorithmClass *>(source);
  AllPurposeProgressAccumulator *self = 
    static_cast<AllPurposeProgressAccumulator *>(clientdata);

  // Call the appropriate handler
  if(eventId == vtkCommand::ProgressEvent) 
    self->CallbackProgress(source, alg->GetProgress());
  else if(eventId == vtkCommand::EndEvent)
    self->CallbackEnd(source, alg->GetProgress() );
  else if(eventId == vtkCommand::StartEvent)
    self->CallbackStart(source);
}

void 
AllPurposeProgressAccumulator
::CallbackITK(itk::Object *source, const EventType &event)
{
  // Cast to an ITK process object
  itk::ProcessObject *alg = dynamic_cast<itk::ProcessObject *>(source);
  
  // Call the appropriate handler
  if( typeid(event) == typeid(itk::ProgressEvent) )
    CallbackProgress( source, alg->GetProgress() );
  else if( typeid(event) == typeid(itk::StartEvent) )
    CallbackStart(source);
  else if( typeid(event) == typeid(itk::EndEvent) )
    CallbackEnd(source, alg->GetProgress() );
}

void 
AllPurposeProgressAccumulator
::RegisterSource(vtkAlgorithmClass *source, float weight)
{
  // See if the source is already registered
  if(m_Source.find(source) == m_Source.end())
    {
    // Create a progress data for this source
    ProgressData pd;
    pd.RunId = 0;
    pd.Type = VTK;

    // Register callbacks with the source
    vtkCallbackCommand *cbc = vtkCallbackCommand::New();
    cbc->SetCallback(AllPurposeProgressAccumulator::CallbackVTK);
    cbc->SetClientData(this);
    pd.ProgressTag = source->AddObserver(vtkCommand::ProgressEvent, cbc);
    pd.EndTag = source->AddObserver(vtkCommand::EndEvent, cbc);
    pd.StartTag = source->AddObserver(vtkCommand::StartEvent, cbc);
    cbc->Delete();

    // Associate the source with the progress data
    m_Source[source] = pd;
    }

  // Generate the info for the current run
  RunData run;
  run.Weight = weight;
  run.Progress = 0.0;
  run.Started = run.Ended = false;
  m_Source[source].Runs.push_back(run);
}

void 
AllPurposeProgressAccumulator
::RegisterSource(itk::ProcessObject *source, float xWeight)
{
  // See if the source is already registered
  if(m_Source.find(source) == m_Source.end())
    {
    // Create a progress data for this source
    ProgressData pd;
    pd.RunId = 0;
    pd.Type = ITK;

    // Register callbacks with this source
    itk::MemberCommand<Self>::Pointer cmd = itk::MemberCommand<Self>::New();
    cmd->SetCallbackFunction(this, &AllPurposeProgressAccumulator::CallbackITK);
    pd.ProgressTag = source->AddObserver(itk::ProgressEvent(), cmd);
    pd.EndTag = source->AddObserver(itk::EndEvent(), cmd);
    pd.StartTag = source->AddObserver(itk::StartEvent(), cmd);

    // Associate the source with the progress data
    m_Source[source] = pd;
    }

  // Generate the info for the current run
  RunData run;
  run.Weight = xWeight;
  run.Progress = 0.0;
  run.Started = run.Ended = false;
  m_Source[source].Runs.push_back(run);
}
  
void 
AllPurposeProgressAccumulator
::UnregisterSource(itk::ProcessObject *source)
{
  // Unregister ourselves as an observer
  source->RemoveObserver(m_Source[source].ProgressTag);
  source->RemoveObserver(m_Source[source].EndTag);
  source->RemoveObserver(m_Source[source].StartTag);

  // Remove the entry from the hash table
  m_Source.erase(source);
} 
  
void 
AllPurposeProgressAccumulator
::UnregisterSource(vtkAlgorithmClass *source)
{
  // Unregister ourselves as an observer
  source->RemoveObserver(m_Source[source].ProgressTag);
  source->RemoveObserver(m_Source[source].EndTag);
  source->RemoveObserver(m_Source[source].StartTag);

  // Remove the entry from the hash table
  m_Source.erase(source);
} 
  
void 
AllPurposeProgressAccumulator
::UnregisterAllSources()
{
  // Unregister each source
  for(SourceIter it = m_Source.begin(); it != m_Source.end(); ++it)
    {
    if(it->second.Type == VTK)
      {
      vtkAlgorithmClass *vtk = static_cast<vtkAlgorithmClass *>(it->first);
      vtk->RemoveObserver(it->second.ProgressTag);
      vtk->RemoveObserver(it->second.StartTag);
      vtk->RemoveObserver(it->second.EndTag);
      }
    else if(it->second.Type == ITK)
      {
      ProcessObject *itk = static_cast<ProcessObject *>(it->first);
      itk->RemoveObserver(it->second.ProgressTag);
      itk->RemoveObserver(it->second.StartTag);
      itk->RemoveObserver(it->second.EndTag);
      }
    }

  // Clear the hash table
  m_Source.clear();

  // Reset the state as well
  ResetProgress();
}

void
AllPurposeProgressAccumulator
::ComputeTotalProgressAndState()
{
  double totalProgress = 0.0, totalWeight = 0.0;
  m_Started = false; m_Ended = true;
  
  for(SourceIter it = m_Source.begin(); it != m_Source.end(); ++it)
    {
    for(unsigned int i = 0; i < it->second.Runs.size(); i++)
      {
      // Accumulate the weights
      double w = it->second.Runs[i].Weight;
      double p = it->second.Runs[i].Progress;
      totalWeight += w;
      totalProgress += w * p;

      // Accumulate the status
      if(it->second.Runs[i].Started)
        m_Started = true;
      if(!it->second.Runs[i].Ended)
        m_Ended = false;
      }    
    }

  double progress = totalWeight > 0.0 ? totalProgress / totalWeight : 0.0;
  SetProgress( (float) progress );
}

void 
AllPurposeProgressAccumulator
::DebugPrint(void *source, const char *state, double p)
{
  /*
  if(m_Source[source].Type == ITK)
    {
    ProcessObject *itk = static_cast<ProcessObject *>(source);
    cout << itk->GetNameOfClass();
    }
  else
    {
    vtkAlgorithm *vtk = static_cast<vtkAlgorithm *>(source);
    cout << vtk->GetClassName();
    }
  cout << " [" << source << "], Run " << m_Source[source].RunId <<
   " of " << m_Source[source].Runs.size() << " : " << state 
   << "; val = " << p << endl;
   */
}

