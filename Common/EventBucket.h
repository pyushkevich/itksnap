#ifndef EVENTBUCKET_H
#define EVENTBUCKET_H

#include "SNAPEvents.h"
#include <set>
#include <iostream>

/**
  A simple 'bucket' that stores events. You can easily add events to
  the bucket and check if events are present there.
  */
class EventBucket
{
public:

  EventBucket();

  virtual ~EventBucket();

  void PutEvent(const itk::EventObject &evt);

  bool HasEvent(const itk::EventObject &evt) const;

  bool IsEmpty() const;

  void Clear();

  friend std::ostream& operator<<(std::ostream& sink, const EventBucket& eb);

protected:

  typedef std::set<std::string> BucketType;
  typedef BucketType::iterator BucketIt;

  BucketType m_Bucket;

};

// IO operator
std::ostream& operator<<(std::ostream& sink, const EventBucket& eb);

#endif // EVENTBUCKET_H
