#include "WrapperBase.h"
#include <itksys/SystemTools.hxx>

unsigned long GlobalImageWrapperIndex = 0ul;

WrapperBase::WrapperBase()
{
  m_UniqueId = ++GlobalImageWrapperIndex;
  m_Initialized = false;
  m_Alpha = 0.5;
}

void
WrapperBase::SetFileName(const std::string &name)
{
  m_FileName = name;
  m_FileNameShort = itksys::SystemTools::GetFilenameWithoutExtension(
        itksys::SystemTools::GetFilenameName(name));
  this->Modified();
  this->InvokeEvent(WrapperMetadataChangeEvent());
}

const std::string &
WrapperBase::GetNickname() const
{
  if (m_CustomNickname.length())
    return m_CustomNickname;
  else if (m_FileName.length())
    return m_FileNameShort;
  else
    return m_DefaultNickname;
}

void
WrapperBase::SetCustomNickname(const std::string &nickname)
{
  if (m_CustomNickname == nickname || (m_CustomNickname.empty() && nickname == m_FileNameShort))
    return;

  if (nickname == m_FileNameShort)
    m_CustomNickname.clear();
  else
    m_CustomNickname = nickname;

  this->Modified();
  this->InvokeEvent(WrapperMetadataChangeEvent());
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
