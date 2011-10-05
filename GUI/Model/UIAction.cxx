#include "UIAction.h"
#include <cstdarg>


void UIAbstractAction::Initialize(GlobalUIModel *model)
{
  this->m_Model = model;
}

IRISWarning::IRISWarning()
  : IRISException()
{

}

IRISWarning::IRISWarning(const char *message, ...)
  : IRISException()
{
  char buffer[1024];
  va_list args;
  va_start(args, message);
  vsprintf(buffer,message,args);
  va_end (args);
  m_SimpleMessage = buffer;
}
