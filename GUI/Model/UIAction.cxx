#include "UIAction.h"
#include <cstdarg>


void UIAbstractAction::Initialize(GlobalUIModel *model)
{
  this->m_Model = model;
}

