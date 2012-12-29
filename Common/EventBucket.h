#ifndef EVENTBUCKET_H
#define EVENTBUCKET_H

#include "SNAPEvents.h"
#include <set>
#include <iostream>

/**
  A simple 'bucket' that stores events. You can easily add events to
  the bucket and check if events are present there.

  TODO: the bucket should keep track of objects that fired the event,
  not only the event types!
  */
class EventBucket
{
public:

  EventBucket();

  virtual ~EventBucket();

  /**
   * @brief Add an event to the bucket.
   */
  void PutEvent(const itk::EventObject &evt, const itk::Object *source);

  /**
   * @brief Check if the bucket has an event from a source (or from all
   * sources if the second parameter has the default value of NULL). This
   * method checks for child events, so if AEvent is a parent BEvent and the
   * bucket holds a BEvent, then HasEvent(AEvent()) will return true.
   */
  bool HasEvent(const itk::EventObject &evt, const itk::Object *source = NULL) const;

  bool IsEmpty() const;

  void Clear();

  friend std::ostream& operator<<(std::ostream& sink, const EventBucket& eb);

protected:

  /**
   * The bucket entry consists of a pointer to the event, which the
   * bucket owns and destroys when it is destroyed or cleared, and the
   * pointer to the originator the event.
   */
  typedef std::pair<itk::EventObject *, const itk::Object *> BucketEntry;
  typedef std::set<BucketEntry> BucketType;
  typedef BucketType::iterator BucketIt;

  BucketType m_Bucket;

};

// IO operator
std::ostream& operator<<(std::ostream& sink, const EventBucket& eb);

#endif // EVENTBUCKET_H
