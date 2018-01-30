#include "LayerIterator.h"
#include "GenericImageData.h"
#include "ImageWrapperBase.h"


LayerIterator
::LayerIterator(
    GenericImageData *data, int role_filter)
{
  // Store the source information
  m_ImageData = data;
  m_RoleFilter = role_filter;

  // Populate role names
  if(m_RoleDefaultNames.size() == 0)
    {
    m_RoleDefaultNames.insert(std::make_pair(MAIN_ROLE, "Main Image"));
    m_RoleDefaultNames.insert(std::make_pair(OVERLAY_ROLE, "Overlay"));
    m_RoleDefaultNames.insert(std::make_pair(LABEL_ROLE, "Segmentation"));
    m_RoleDefaultNames.insert(std::make_pair(SNAP_ROLE, "SNAP Image"));
    }

  // Move to the beginning
  MoveToBegin();
}

LayerIterator& LayerIterator
::MoveToBegin()
{
  // Initialize to point to the first wrapper in the first role, even if
  // this is an invalid configuration
  m_RoleIter = m_ImageData->m_Wrappers.begin();
  if(m_RoleIter != m_ImageData->m_Wrappers.end())
    m_WrapperInRoleIter = m_RoleIter->second.begin();

  // Move up until we find a valid role or end
  while(!IsAtEnd() && !IsPointingToListableLayer())
    {
    MoveToNextTrialPosition();
    }

  return *this;
}

bool LayerIterator
::IsAtEnd() const
{
  // We are at end when there are no roles left
  return m_RoleIter == m_ImageData->m_Wrappers.end();
}


LayerIterator& LayerIterator
::MoveToEnd()
{
  m_RoleIter = m_ImageData->m_Wrappers.end();
  return *this;
}

LayerIterator& LayerIterator
::Find(ImageWrapperBase *value)
{
  // Just a linear search - we won't have so many wrappers!
  MoveToBegin();
  while(!this->IsAtEnd() && this->GetLayer() != value)
    ++(*this);
  return *this;
}

LayerIterator &LayerIterator::Find(unsigned long layer_id)
{
  // Just a linear search - we won't have so many wrappers!
  MoveToBegin();
  while(!this->IsAtEnd() && this->GetLayer() && this->GetLayer()->GetUniqueId() != layer_id)
    ++(*this);
  return *this;
}

void LayerIterator::MoveToNextTrialPosition()
{
  // If we are at the end of storage, that's it
  if(m_RoleIter == m_ImageData->m_Wrappers.end())
    return;

  // If we are at the end of a chain of wrappers, or if the current role
  // is not a valid role, go to the start of the next role
  else if(m_WrapperInRoleIter == m_RoleIter->second.end() ||
     !(m_RoleFilter & m_RoleIter->first))
    {
    ++m_RoleIter;
    if(m_RoleIter != m_ImageData->m_Wrappers.end())
      m_WrapperInRoleIter = m_RoleIter->second.begin();

    }

  // Otherwise, advance the iterator in the wrapper chain
  else
    ++m_WrapperInRoleIter;
}

bool LayerIterator::IsPointingToListableLayer() const
{
  // I split this up for debugging

  // Are we at end of roles?
  if(m_RoleIter == m_ImageData->m_Wrappers.end())
    return false;

  // Are we in a valid role?
  LayerRole lr = m_RoleIter->first;
  if((m_RoleFilter & lr) == 0)
    return false;

  // In our role, are we at the end?
  if(m_WrapperInRoleIter == m_RoleIter->second.end())
    return false;

  // Is the layer null?
  if((*m_WrapperInRoleIter).IsNull())
    return false;

  return true;
}

LayerIterator &
LayerIterator::operator ++()
{
  do
    {
    MoveToNextTrialPosition();
    }
  while(!IsAtEnd() && !IsPointingToListableLayer());

  return *this;
}

LayerIterator &
LayerIterator::operator +=(int k)
{
  for(int i = 0; i < k; i++)
    ++(*this);
  return *this;
}

ImageWrapperBase * LayerIterator::GetLayer() const
{
  assert(IsPointingToListableLayer());
  return (*m_WrapperInRoleIter);
}

ScalarImageWrapperBase * LayerIterator::GetLayerAsScalar() const
{
  return dynamic_cast<ScalarImageWrapperBase *>(this->GetLayer());
}

VectorImageWrapperBase * LayerIterator::GetLayerAsVector() const
{
  return dynamic_cast<VectorImageWrapperBase *>(this->GetLayer());
}

LayerRole
LayerIterator::GetRole() const
{
  assert(IsPointingToListableLayer());
  return m_RoleIter->first;
}

int LayerIterator::GetPositionInRole() const
{
  return (int)(m_WrapperInRoleIter - m_RoleIter->second.begin());
}

int LayerIterator::GetNumberOfLayersInRole()
{
  assert(IsPointingToListableLayer());
  return m_RoleIter->second.size();
}

bool LayerIterator::IsFirstInRole() const
{
  assert(IsPointingToListableLayer());
  int pos = m_WrapperInRoleIter - m_RoleIter->second.begin();
  return (pos == 0);
}

bool LayerIterator::IsLastInRole() const
{
  assert(IsPointingToListableLayer());
  int pos = m_WrapperInRoleIter - m_RoleIter->second.begin();
  return (pos == m_RoleIter->second.size() - 1);
}

bool LayerIterator::operator ==(const LayerIterator &it)
{
  // Two iterators are equal if they both point to the same location
  // or both are at the end.
  if(this->IsAtEnd())
    return it.IsAtEnd();
  else if(it.IsAtEnd())
    return false;
  else
    return this->GetLayer() == it.GetLayer();
}

bool LayerIterator::operator !=(const LayerIterator &it)
{
  return !((*this) == it);
}

std::map<LayerRole, std::string> LayerIterator::m_RoleDefaultNames;


void LayerIterator::Print(const char *what) const
{
  std::cout << "LI with filter " << m_RoleFilter << " operation " << what << std::endl;
  if(this->IsAtEnd())
    {
    std::cout << "  AT END" << std::endl;
    }
  else
    {
    std::cout << "  Role:         " << m_RoleDefaultNames[this->GetRole()] << std::endl;
    std::cout << "  Pos. in Role: "
              << (int)(m_WrapperInRoleIter - m_RoleIter->second.begin()) << " of "
              << (int) m_RoleIter->second.size() << std::endl;
    std::cout << "  Valid:        " << this->IsPointingToListableLayer() << std::endl;
    }
}
