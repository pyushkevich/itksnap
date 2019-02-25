#include "TagList.h"
#include <algorithm>
#include <sstream>

TagList::TagList()
{

}

bool TagList::Contains(const std::string &tag) const
{
  return std::find(this->begin(), this->end(), tag) != this->end();
}

bool TagList::AddTag(const std::string &tag)
{
  if(!this->Contains(tag))
    {
    this->push_back(tag);
    return true;
    }
  else
    {
    return false;
    }
}

bool TagList::RemoveTag(const std::string &tag)
{
  if(this->Contains(tag))
    {
    this->remove(tag);
    return true;
    }
  else
    {
    return false;
    }
}

std::string TagList::ToString() const
{
  // Add all the items in the container
  std::ostringstream oss;
  for(Superclass::const_iterator it = this->begin(); it != this->end(); ++it)
    {
    if(it != this->begin())
      oss << ",";
    oss << (*it);
    }

  return oss.str();
}
