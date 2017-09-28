#ifndef __ProcessEventsITKCommand_h_
#define __ProcessEventsITKCommand_h_

#include <QCoreApplication>
#include <itkCommand.h>

/**
 * This is an ITK command that can be passed to ITK filters as an iteration or
 * progress observer. When the command is called, it executes
 * QCoreApplication::processEvents(), i.e., responds to user actions
 */
class ProcessEventsITKCommand : public itk::Command
{
public:
  /** Standard class typedefs. */
  typedef ProcessEventsITKCommand Self;
  typedef itk::SmartPointer< Self >     Pointer;

  /** Run-time type information (and related methods). */
  itkTypeMacro(ProcessEventsITKCommand, itk::Command)

  /** Method for creation through the object factory. */
  itkNewMacro(Self)

  /** Abstract method that defines the action to be taken by the command. */
  virtual void Execute(itk::Object *caller, const itk::EventObject & event) ITK_OVERRIDE
  {
    QCoreApplication::processEvents();
  }

  /** Abstract method that defines the action to be taken by the command.
   * This variant is expected to be used when requests comes from a
   * const Object */
  virtual void Execute(const itk::Object *caller, const itk::EventObject & event) ITK_OVERRIDE
  {
    QCoreApplication::processEvents();
  }
};


#endif // __ProcessEventsITKCommand_h_
