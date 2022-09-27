/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: AllPurposeProgressAccumulator.h,v $
  Language:  C++
  Date:      $Date: 2010/06/28 18:45:08 $
  Version:   $Revision: 1.2 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __AllPurposeProgressAccumulator_h_
#define __AllPurposeProgressAccumulator_h_

#include <map>
#include <vector>
#include "SNAPCommon.h"
#include "itkProcessObject.h"
#include "itkCommand.h"

// Some ugly code here because VTK decided to change its architecture 
// from 4.2 to 4.4. 
#include "vtkConfigure.h"
#if (VTK_MAJOR_VERSION == 4) && (VTK_MINOR_VERSION <= 4)
  #include "vtkProcessObject.h"
  #define vtkAlgorithmClass vtkProcessObject
#else
  #include "vtkAlgorithm.h"
  #define vtkAlgorithmClass vtkAlgorithm
#endif

/**
 * @brief DoNothingCommand is a simple placeholder to execute methods
 * that requires a itk::command to run, and a nullptr will cause crash.
 *
 * It is better to be used via DoNothingCommandSingleton class
 */
class DoNothingCommand : public itk::Command
{
public:
	irisITKObjectMacro(DoNothingCommand, itk::Command)
	void Execute(const Object *, const itk::EventObject &) override {}
	void Execute(Object *, const itk::EventObject &) override {}
protected:
	DoNothingCommand() {}
	virtual ~DoNothingCommand() {}
};

/**
 * @brief DoNothingCommandSingleton maintain one instance of DoNothingCommand
 * to avoid creating multiple instances that are really not necessary.
 */
class DoNothingCommandSingleton
{
public:
	~DoNothingCommandSingleton() {}
	DoNothingCommandSingleton(const DoNothingCommandSingleton& other) = delete;
	void operator=(const DoNothingCommandSingleton& other) = delete;

	static DoNothingCommandSingleton &GetInstance()
	{
		static DoNothingCommandSingleton instance;
		return instance;
	}

	DoNothingCommand *GetCommand()
	{ return m_DoNothingCommand.GetPointer(); }

private:
	DoNothingCommandSingleton()
	{
		m_DoNothingCommand = DoNothingCommand::New();
	}
	SmartPtr<DoNothingCommand> m_DoNothingCommand;
};

class TrivalProgressSource;

/**
 * \class AllPurposeProgressAccumulator
 * \brief This class combines progress reports from different sources into
 * a single progress value.
 *
 * This class works as follows. A VTK or ITK process object can register with
 * this progress accumulator one or more times with different weights. When the
 * process object runs, the progress reported by it is accumulated into a total
 * progress value. The weights you supply do not need to add up to one, they
 * will be normalized automatically
 *
 * Because a source may fire more than one Start-Progress-End sequence per
 * execution, you have to advance multiple runs for one source manually using
 * the method StartNextRun.
 */
class AllPurposeProgressAccumulator : public itk::ProcessObject
{
public:
  /** Standard class typedefs. */
  typedef AllPurposeProgressAccumulator        Self;
  typedef itk::Object                          Superclass;
  typedef itk::SmartPointer<Self>              Pointer;
  typedef itk::SmartPointer<const Self>        ConstPointer;

  typedef itk::EventObject                     EventType;

  typedef itk::SmartPointer<itk::Command>      CommandPointer;

  /** Standard New method. */
  itkNewMacro(Self)

  /** Runtime information support. */
  itkTypeMacro(AllPurposeProgressAccumulator, Object)

  /** 
   * Reset the progress meter. This should be done before the entire pipeline
   * is executed for a second time. The progress will be set to zero, and the
   * first Begin event received by this class will be forwared on to the
   * observers
   */
  void ResetProgress() ;

  /**
   * Start the next run for a source that has been registered for multiple runs.
   * This should be done in a pipeline that uses the same source multiple times.
   */
  void StartNextRun(void *source);

  /** 
   * Add a VTK algorithm to the list of monitored objects
   */
  void RegisterSource(vtkAlgorithmClass *source, float xWeight);

  /** 
   * Add an ITK algorithm to the list of monitored objects
   */
  void RegisterSource(itk::ProcessObject *source, float xWeight);

  /** Unregister a source (and all runs associated with it) */
  void UnregisterSource(vtkAlgorithmClass *source);

  /** Unregister a source (and all runs associated with it) */
  void UnregisterSource(itk::ProcessObject *source);
  
  /** Unregister all sources and all runs */
  void UnregisterAllSources();

  /** Generic callback function for generic sources */
  static void GenericProgressCallback(void *source, double progress);

  /**
   * Register a generic source. This source will communicate its progress
   * using the function GenericProgressCallback. You must specify the number
   * of times this generic source will go through the progress cycle.
   *
   * The function returns the void * that should be passed used when calling
   * the callback and when unregistering the source. To avoid memory leaks, the
   * generic source must be unregitered.
   */
  void *RegisterGenericSource(int n_runs, float total_weight);

  /**
   * Register a source by creating a command that will be used to observe progress
   * from that source. This is an alternative way to add itk sources to this object.
   */
  CommandPointer RegisterITKSourceViaCommand(float xWeight);

  /**
   * Unregister a generic source and free associated memory
   */
  void UnregsterGenericSource(void *source);

protected:

  AllPurposeProgressAccumulator();

private:

  // Source types
  enum SourceType { ITK, VTK, GENERIC };

  // Data structure associated with each component that we listen to
  struct RunData
    {
    double Weight, Progress;
    bool Started, Ended;
    };
  
  struct ProgressData
    {
    std::vector<RunData> Runs;
    unsigned int RunId;
    unsigned long StartTag, EndTag, ProgressTag;
    SourceType Type;
    };

  // Callbacks passed to ITK and VTK sources
  static void CallbackVTK(
    vtkObject *source, unsigned long eventId, void *clientdata, void *callData);
  void CallbackITK(itk::Object *comp, const EventType &event);

  // Common event-based callbacks
  void CallbackStart(void *source);
  void CallbackEnd(void *source, double progress);
  void CallbackProgress(void *source, double progress);

  // Compute total progress
  void ComputeTotalProgressAndState();

  void DebugPrint(void *, const char *, double prog = 0.0);

  // A map of weights and filter pointers
  typedef std::map<void *, ProgressData> SourceMap;
  typedef SourceMap::iterator SourceIter;
  SourceMap m_Source;

  // A map of ITK commands and respective trivial sources
  typedef std::map<CommandPointer, itk::SmartPointer<TrivalProgressSource> > CommandToTrivialSourceMap;
  CommandToTrivialSourceMap m_CommandToTrivialSourceMap;

  // The overall state of the entire pipeline
  bool m_Started, m_Ended;

  // Helper class used with generic sources
  class GenericProgressSource
  {
  public:
    static void callback(void *p, double progress);
    GenericProgressSource(AllPurposeProgressAccumulator *parent);
    void StartNextRun();
  protected:
    AllPurposeProgressAccumulator *m_Parent;
    bool m_Started, m_Ended;
  };

};

/**
 * @brief This class can be used to generate progress in a non-ITK function
 * or class. When you call commands StartProgress, SetProgress or AddProgress,
 * the command will generate corresponding ITK events.
 */
class TrivalProgressSource : public itk::ProcessObject
{
public:
  /** Standard class typedefs. */
  typedef TrivalProgressSource                 Self;
  typedef itk::ProcessObject                   Superclass;
  typedef itk::SmartPointer<Self>              Pointer;
  typedef itk::SmartPointer<const Self>        ConstPointer;

  typedef itk::EventObject                     EventType;

  /** Standard New method. */
  itkNewMacro(Self)

  /** Runtime information support. */
  itkTypeMacro(TrivalProgressSource, Object)

  /** Start progress */
  void StartProgress(double max_progress = 1.0);

  /** Add some progress */
  void AddProgress(double delta);

  /** Set current progress */

  /** Finish */
  void EndProgress();

  /** A callback that is used in conjunction with AllPurposeProgressAccumulator */
  void Callback(itk::Object *source, const itk::EventObject &event);

  /** Add observer to all 3 progress related events */
  void AddObserverToProgressEvents(itk::Command *cmd);

protected:
  TrivalProgressSource();

private:

  double m_MaxProgress;

};

class ImageReadingProgressAccumulator: public AllPurposeProgressAccumulator
{
public:
	irisITKObjectMacro(ImageReadingProgressAccumulator, AllPurposeProgressAccumulator)

	void AddProgressReporterCommand(itk::Command *cmd)
	{
    this->AddObserver(itk::StartEvent(), cmd);
		this->AddObserver(itk::ProgressEvent(), cmd);
    this->AddObserver(itk::EndEvent(), cmd);
	}

	/**
	 * @brief GetHeaderProgressCommand, return an itk::Command for tracking image
	 * header reading progress
	 */
	SmartPtr<itk::Command> GetHeaderProgressCommand()
	{ return this->m_HeaderProgressCommand; }

	/**
	 * @brief GetDataProgressCommand, return an itk::Command for tracking image
	 * data reading progress
	 */
	SmartPtr<itk::Command> GetDataProgressCommand()
	{ return this->m_DataProgressCommand; }

	/**
	 * @brief GetMiscProgressCommand, return an itk::Command for tracking progress
	 * other than image header and data
	 */
	SmartPtr<itk::Command> GetMiscProgressCommand()
	{ return this->m_MiscProgressCommand; }


protected:
	ImageReadingProgressAccumulator()
	{
		m_HeaderProgressCommand = RegisterITKSourceViaCommand(0.1);
		m_DataProgressCommand = RegisterITKSourceViaCommand(0.85);
		m_MiscProgressCommand = RegisterITKSourceViaCommand(0.05);
	}

	virtual ~ImageReadingProgressAccumulator() {}

	SmartPtr<itk::Command> m_HeaderProgressCommand;
	SmartPtr<itk::Command> m_DataProgressCommand;
	SmartPtr<itk::Command> m_MiscProgressCommand;
};


#endif
