#include "WrapperBase.h"

unsigned long GlobalImageWrapperIndex = 0ul;

WrapperBase::WrapperBase()
{
  // Set the unique wrapper id
  m_UniqueId = ++GlobalImageWrapperIndex;
}

void
WrapperBase::SetUserData(const std::string &role, itk::Object *data)
{
  m_UserDataMap[role] = data;
}

itk::Object *
WrapperBase::GetUserData(const std::string &role) const
{
  UserDataMapType::const_iterator it = m_UserDataMap.find(role);
  if(it == m_UserDataMap.end())
    return NULL;
  else return it->second;
}

unsigned long
WrapperBase::GetUniqueId() const
{
  return m_UniqueId;
}
