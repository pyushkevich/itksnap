// Test: Rebroadcaster forwards events from a source object as events of a
// target object.
//
// Regression test: when an identical rebroadcast already existed, Rebroadcast()
// returned its tag without checking that the observer was still attached. Code
// that removed the observer with RemoveObserver(tag) and later rebroadcast
// from the same object again (switching the active mesh data array, or setting
// the same color map twice) got a dead tag, and events were no longer
// forwarded.

#include "Rebroadcaster.h"
#include "itkCommand.h"
#include "itkObject.h"
#include <iostream>

static int n_failures = 0;

static void Check(bool cond, const char *what)
{
  if(!cond)
    {
    std::cerr << "FAILED: " << what << std::endl;
    ++n_failures;
    }
}

// Counts the events fired by an object
struct Counter
{
  int count = 0;
  bool callback_seen_first = true;
  const bool *callback_flag = nullptr;

  void Watch(itk::Object *obj)
  {
    obj->AddObserver(itk::UserEvent(), [this](const itk::EventObject &) {
      ++count;
      if(callback_flag && !*callback_flag)
        callback_seen_first = false;
    });
  }
};

int main(int, char *[])
{
  // Basic forwarding, and deduplication of identical requests
  {
    auto src = itk::Object::New(), trg = itk::Object::New();
    Counter c;
    c.Watch(trg);
    unsigned long t1 = Rebroadcaster::Rebroadcast(src, itk::ModifiedEvent(), trg, itk::UserEvent());
    unsigned long t2 = Rebroadcaster::Rebroadcast(src, itk::ModifiedEvent(), trg, itk::UserEvent());
    Check(t1 == t2, "duplicate rebroadcast returns the same tag");
    src->InvokeEvent(itk::ModifiedEvent());
    Check(c.count == 1, "event forwarded exactly once");
  }

  // Observer removed behind the rebroadcaster's back, then rebroadcast again
  {
    auto src = itk::Object::New(), trg = itk::Object::New();
    Counter c;
    c.Watch(trg);
    unsigned long tag = Rebroadcaster::Rebroadcast(src, itk::ModifiedEvent(), trg, itk::UserEvent());
    src->RemoveObserver(tag);
    Rebroadcaster::Rebroadcast(src, itk::ModifiedEvent(), trg, itk::UserEvent());
    src->InvokeEvent(itk::ModifiedEvent());
    Check(c.count == 1, "rebroadcast works again after RemoveObserver");
  }

  // StopRebroadcasting, then rebroadcast again
  {
    auto src = itk::Object::New(), trg = itk::Object::New();
    Counter c;
    c.Watch(trg);
    unsigned long tag = Rebroadcaster::Rebroadcast(src, itk::ModifiedEvent(), trg, itk::UserEvent());
    Rebroadcaster::StopRebroadcasting(src, tag);
    src->InvokeEvent(itk::ModifiedEvent());
    Check(c.count == 0, "no event after StopRebroadcasting");
    Rebroadcaster::Rebroadcast(src, itk::ModifiedEvent(), trg, itk::UserEvent());
    src->InvokeEvent(itk::ModifiedEvent());
    Check(c.count == 1, "rebroadcast works again after StopRebroadcasting");
  }

  // The callback runs before the target event is fired, and a duplicate
  // request adopts the callback when the existing one has none
  {
    auto src = itk::Object::New(), trg = itk::Object::New();
    bool callback_ran = false;
    Counter c;
    c.callback_flag = &callback_ran;
    c.Watch(trg);
    Rebroadcaster::Rebroadcast(src, itk::ModifiedEvent(), trg, itk::UserEvent());
    Rebroadcaster::Rebroadcast(src, itk::ModifiedEvent(), trg, itk::UserEvent(),
                               [&callback_ran](const itk::Object *, const itk::EventObject &) {
                                 callback_ran = true;
                               });
    src->InvokeEvent(itk::ModifiedEvent());
    Check(callback_ran, "duplicate request adopts the callback");
    Check(c.count == 1 && c.callback_seen_first, "callback runs before the target event");
  }

  // Deleting the target stops the rebroadcast without crashing
  {
    auto src = itk::Object::New();
    {
      auto trg = itk::Object::New();
      Rebroadcaster::Rebroadcast(src, itk::ModifiedEvent(), trg, itk::UserEvent());
    }
    src->InvokeEvent(itk::ModifiedEvent());

    // A new rebroadcast from the same source still works
    auto trg2 = itk::Object::New();
    Counter c;
    c.Watch(trg2);
    Rebroadcaster::Rebroadcast(src, itk::ModifiedEvent(), trg2, itk::UserEvent());
    src->InvokeEvent(itk::ModifiedEvent());
    Check(c.count == 1, "rebroadcast from a source whose old target was deleted");
  }

  if(n_failures)
    {
    std::cerr << n_failures << " check(s) failed" << std::endl;
    return 1;
    }
  std::cout << "All Rebroadcaster checks passed" << std::endl;
  return 0;
}
