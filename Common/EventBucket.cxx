#include "EventBucket.h"

EventBucket::EventBucket()
{
}

EventBucket::~EventBucket()
{
  Clear();
}

void EventBucket::Clear()
{
  m_Bucket.clear();
}

bool EventBucket::HasEvent(const itk::EventObject &evt) const
{
  std::string name(evt.GetEventName());
  return m_Bucket.find(name) != m_Bucket.end();
}

bool EventBucket::IsEmpty() const
{
  return m_Bucket.size() == 0;
}

void EventBucket::PutEvent(const itk::EventObject &evt)
{
  std::string name(evt.GetEventName());
  m_Bucket.insert(name);
}

std::ostream& operator<<(std::ostream& sink, const EventBucket& eb)
{
  sink << "EventBucket[";
  for(EventBucket::BucketIt it = eb.m_Bucket.begin(); it != eb.m_Bucket.end();)
    {
    sink << *it;
    if(++it != eb.m_Bucket.end())
      sink << ", ";
    }
  sink << "]";
  return sink;
}

