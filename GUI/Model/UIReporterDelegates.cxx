#include "UIReporterDelegates.h"
#include "itkProcessObject.h"
#include "itkCommand.h"

void ProgressReporterDelegate
::ProgressCallback(itk::Object *source, const itk::EventObject &event)
{
  itk::ProcessObject *po = static_cast<itk::ProcessObject *>(source);
  this->SetProgressValue(po->GetProgress());
}

SmartPtr<itk::Command> ProgressReporterDelegate::CreateCommand()
{
  SmartPtr<itk::MemberCommand<ProgressReporterDelegate> > progcmd =
      itk::MemberCommand<ProgressReporterDelegate>::New();

  progcmd->SetCallbackFunction(this, &ProgressReporterDelegate::ProgressCallback);

  SmartPtr<itk::Command> return_cmd = progcmd.GetPointer();
  return return_cmd;
}
