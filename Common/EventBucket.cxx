#include "EventBucket.h"

unsigned long EventBucket::m_GlobalMTime = 1;

EventBucket::EventBucket()
{
  m_MTime = m_GlobalMTime++;
}

EventBucket::~EventBucket()
{
  Clear();
}

void EventBucket::Clear()
{
  m_Lock.Lock();
  // Remove all the event objects
  for(BucketIt it = m_Bucket.begin(); it != m_Bucket.end(); ++it)
    {
    delete(it->first);
    }
  m_Bucket.clear();
  m_Lock.Unlock();

  m_MTime = m_GlobalMTime++;
}

bool EventBucket::HasEvent(const itk::EventObject &evt, const itk::Object *source) const
{
  m_Lock.Lock();
  // Search for the event. Buckets are never too large so a linear search is fine
  for(BucketIt it = m_Bucket.begin(); it != m_Bucket.end(); ++it)
    {
    const BucketEntry &entry = *it;
    std::string echeck = evt.GetEventName();
    std::string eentry = entry.first->GetEventName();
    if(evt.CheckEvent(entry.first) && (source == NULL || source == entry.second))
      {
      m_Lock.Unlock();
      return true;
      }
    }

  m_Lock.Unlock();
  return false;
}

bool EventBucket::IsEmpty() const
{
  return m_Bucket.size() == 0;
}

void EventBucket::PutEvent(const itk::EventObject &evt, const itk::Object *source)
{
  if(!this->HasEvent(evt, source))
    {
    BucketEntry entry;
    entry.first = evt.MakeObject();
    entry.second = source;
    m_Lock.Lock();
    m_Bucket.insert(entry);
    m_Lock.Unlock();
    m_MTime = m_GlobalMTime++;
    }
}

std::ostream& operator<<(std::ostream& sink, const EventBucket& eb)
{
  sink << "EventBucket[";
  for(EventBucket::BucketIt it = eb.m_Bucket.begin(); it != eb.m_Bucket.end();)
    {
    sink << it->first->GetEventName() << "(" << it->second << ")";
    if(++it != eb.m_Bucket.end())
      sink << ", ";
    }
  sink << "]";
  return sink;
}

