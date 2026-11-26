#ifndef CURSORHISTORYMODEL_H
#define CURSORHISTORYMODEL_H

#include "AbstractModel.h"
#include "IRISVectorTypes.h"
#include <functional>
#include <vector>

class GlobalUIModel;

/**
  This model keeps a history of the places the cursor has been, so that the
  user can go back and forward between them, like in a web browser, or reset
  the cursor to the center of the image. The time point is treated as one of
  the cursor coordinates.

  We don't want every intermediate position recorded when the user drags the
  cursor or scrolls through slices. So the last entry keeps getting updated
  while the cursor is moving, and a new entry is only created after the mouse
  button is released, or after the cursor has been still for a second. We also
  don't record anything while 4D replay is running.

  The history is cleared when a new main image is loaded. The UI should call
  SetPointerDown when the mouse button is pressed and released in the views.
  */
class CursorHistoryModel : public AbstractModel
{
public:
  irisITKObjectMacro(CursorHistoryModel, AbstractModel)

  /**
    States in which the model can be, which allow the activation and
    deactivation of the back, forward and reset buttons
    */
  enum UIState
  {
    UIF_CAN_GO_BACK,
    UIF_CAN_GO_FORWARD,
    UIF_CAN_RESET
  };

  /** Check the state flags above */
  bool CheckState(UIState state);

  /** Set the parent model */
  void SetParentModel(GlobalUIModel *parent);

  /** Go to the previous place in the history */
  void GoBack();

  /** Go to the next place in the history */
  void GoForward();

  /** Move the cursor to the center of the main image */
  void Reset();

  /** The UI calls this when the mouse button is pressed or released in a view */
  void SetPointerDown(bool down);

  /**
    Make the next cursor change start a new entry, even if it comes right
    after the last one. Use this before jumping to a new position.
    */
  void CloseCurrentEntry() { m_EntryOpen = false; }

  /** How long the cursor has to be still before a move starts a new entry */
  static constexpr double PAUSE_SECONDS = 1.0;

  /** Maximum number of entries in the history */
  static constexpr unsigned int MAX_ENTRIES = 50;

  /** Set the clock used to time cursor changes, in seconds (for testing) */
  void SetClock(std::function<double()> clock) { m_Clock = clock; }

  /** Number of entries and index of the current entry (for testing) */
  unsigned int GetNumberOfEntries() const { return m_Entries.size(); }
  int GetCurrentIndex() const { return m_Index; }

protected:
  CursorHistoryModel();
  virtual ~CursorHistoryModel() {}

  // An entry in the history. The position is stored in RAS coordinates so
  // that it does not depend on the voxel grid of the reference space
  struct Entry
  {
    Vector3d ras;
    Vector3i voxel;
    unsigned int tp;
  };

  // Get the place where the cursor is now
  Entry GetCurrentPlace() const;

  // Two entries are the same place if they have the same voxel and time point
  static bool IsSamePlace(const Entry &a, const Entry &b)
    { return a.voxel == b.voxel && a.tp == b.tp; }

  // Event handlers
  void OnCursorChange();
  void OnMainImageChange();
  void On4DReplayChange();

  // Add the current place as the newest entry
  void AddCurrentPlace();

  // Move the cursor to the entry with the given index
  void GoTo(int index);

  GlobalUIModel *m_Parent = nullptr;

  // The entries and the index of the current one
  std::vector<Entry> m_Entries;
  int m_Index = -1;

  // Whether cursor changes may still update the newest entry
  bool m_EntryOpen = false;

  // Whether the mouse button is down, whether we are moving the cursor
  // ourselves, and whether 4D replay is running
  bool m_PointerDown = false, m_Navigating = false, m_Suspended = false;

  // Time of the last cursor change
  double m_LastChangeTime = 0.0;

  // The clock, in seconds
  std::function<double()> m_Clock;
};

#endif // CURSORHISTORYMODEL_H
