#include "WrapperBase.h"
#include <itksys/SystemTools.hxx>

unsigned long GlobalImageWrapperIndex = 0ul;

bool WrapperBase::s_HideFileExtensions = true;

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
  m_FileNameShort = itksys::SystemTools::GetFilenameName(name);
  m_FileNameNoExt = StripFilenameExtension(name);
  this->Modified();
  this->InvokeEvent(WrapperMetadataChangeEvent());
}

const std::string &
WrapperBase::GetFileNickname() const
{
  return s_HideFileExtensions ? m_FileNameNoExt : m_FileNameShort;
}

const std::string &
WrapperBase::GetNickname() const
{
  if (m_CustomNickname.length())
    return m_CustomNickname;
  else if (m_FileName.length())
    return GetFileNickname();
  else
    return m_DefaultNickname;
}

void
WrapperBase::NotifyNicknameChanged()
{
  this->Modified();
  this->InvokeEvent(WrapperMetadataChangeEvent());
}

void
WrapperBase::SetCustomNickname(const std::string &nickname)
{
  const std::string &file_nickname = GetFileNickname();
  if (m_CustomNickname == nickname || (m_CustomNickname.empty() && nickname == file_nickname))
    return;

  if (nickname == file_nickname)
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
