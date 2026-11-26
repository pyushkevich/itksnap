#include "CursorHistoryModel.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "LabelImageWrapper.h"
#include "GlobalState.h"
#include "SNAPEvents.h"
#include "SNAPEventListenerCallbacks.h"
#include <chrono>

CursorHistoryModel::CursorHistoryModel()
{
  // Use the system clock by default
  m_Clock = []() {
    auto t = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration<double>(t).count();
  };
}

void CursorHistoryModel::SetParentModel(GlobalUIModel *parent)
{
  m_Parent = parent;
  IRISApplication *app = parent->GetDriver();

  // We need to know when each cursor change happens, so we listen to the
  // events directly rather than waiting for the model to be updated. The
  // cursor update event is also fired when the time point changes
  AddListener(app, CursorUpdateEvent(), this, &Self::OnCursorChange);
  AddListener(app, MainImageDimensionsChangeEvent(), this, &Self::OnMainImageChange);
  AddListener(app->GetGlobalState()->Get4DReplayModel(), ValueChangedEvent(),
              this, &Self::On4DReplayChange);
}

CursorHistoryModel::Entry CursorHistoryModel::GetCurrentPlace() const
{
  GenericImageData *gid = m_Parent->GetDriver()->GetCurrentImageData();
  Entry e;
  e.ras = gid->GetCursorPositionRAS();
  e.tp = gid->GetCursorTimePoint();
  return e;
}

bool CursorHistoryModel::IsSamePlace(const Entry &a, const Entry &b) const
{
  // The places are compared in the voxel grid of the active segmentation
  LabelImageWrapper *seg = m_Parent->GetDriver()->GetSelectedSegmentationLayer();
  if(a.tp != b.tp || !seg)
    return a.tp == b.tp && a.ras == b.ras;

  Vector3d va = seg->TransformNIFTICoordinatesToVoxelCIndex(a.ras);
  Vector3d vb = seg->TransformNIFTICoordinatesToVoxelCIndex(b.ras);
  for(unsigned int d = 0; d < 3; d++)
    if(std::round(va[d]) != std::round(vb[d]))
      return false;
  return true;
}

bool CursorHistoryModel::CheckState(UIState state)
{
  bool have_main = m_Parent && m_Parent->GetDriver()->IsMainImageLoaded();
  switch(state)
    {
    case UIF_CAN_GO_BACK:
      // We can also go back if the cursor has moved without being recorded
      return have_main && m_Index >= 0 &&
             (m_Index > 0 || !IsSamePlace(GetCurrentPlace(), m_Entries[m_Index]));
    case UIF_CAN_GO_FORWARD:
      return have_main && m_Index >= 0 && m_Index < (int) m_Entries.size() - 1;
    case UIF_CAN_RESET:
      return have_main;
    }
  return false;
}

void CursorHistoryModel::AddCurrentPlace()
{
  // If the user went back, the entries ahead are dropped, like in a browser
  if(m_Index >= 0)
    m_Entries.resize(m_Index + 1);

  // Add the entry, dropping the oldest one if the history is full
  m_Entries.push_back(GetCurrentPlace());
  if(m_Entries.size() > MAX_ENTRIES)
    m_Entries.erase(m_Entries.begin());
  m_Index = (int) m_Entries.size() - 1;
}

void CursorHistoryModel::OnCursorChange()
{
  IRISApplication *app = m_Parent->GetDriver();
  if(m_Navigating || m_Suspended || !app->IsMainImageLoaded())
    return;

  // How the cursor is being changed (as part of a drag, or not)
  auto kind = app->GetCursorChangeKind();
  Entry place = GetCurrentPlace();
  double now = m_Clock();

  // The event is also fired when the cursor does not actually move, e.g., at
  // the end of a drag, which still closes the entry for the drag
  if(m_Index >= 0 && IsSamePlace(place, m_Entries[m_Index]))
    {
    if(kind == IRISApplication::CURSOR_DRAG_END)
      m_EntryOpen = false;
    return;
    }

  // The newest entry is updated while a drag goes on, and while the cursor
  // keeps being moved in quick succession (scrolling, arrow keys). A drag that
  // starts, or a jump, is always a new place. When the cursor is put back
  // during loading, it is still the place where the image opened
  bool at_newest = m_Index == (int) m_Entries.size() - 1;
  bool can_update = m_Index >= 0 && at_newest && m_EntryOpen;
  bool update = false;
  switch(kind)
    {
    case IRISApplication::CURSOR_LOAD:
      update = m_Index >= 0 && at_newest;
      break;
    case IRISApplication::CURSOR_DRAG:
    case IRISApplication::CURSOR_DRAG_END:
      update = can_update;
      break;
    case IRISApplication::CURSOR_MOVE:
      update = can_update && now - m_LastChangeTime < PAUSE_SECONDS;
      break;
    default:
      break;
    }

  if(update)
    m_Entries[m_Index] = place;
  else
    AddCurrentPlace();

  // The entry stays open for more changes, unless the drag or jump is over
  m_EntryOpen = (kind != IRISApplication::CURSOR_DRAG_END && kind != IRISApplication::CURSOR_JUMP &&
                 kind != IRISApplication::CURSOR_LOAD);
  m_LastChangeTime = now;
  this->InvokeEvent(StateMachineChangeEvent());
}

void CursorHistoryModel::OnMainImageChange()
{
  // Start the history over, with the place where the new image opened
  m_Entries.clear();
  m_Index = -1;
  m_EntryOpen = false;
  if(m_Parent->GetDriver()->IsMainImageLoaded())
    AddCurrentPlace();
  this->InvokeEvent(StateMachineChangeEvent());
}

void CursorHistoryModel::On4DReplayChange()
{
  // Don't record anything during replay. When it stops, record where it
  // stopped as one entry
  m_Suspended = m_Parent->GetDriver()->GetGlobalState()->Get4DReplay();
  if(!m_Suspended)
    {
    m_EntryOpen = false;
    this->OnCursorChange();
    m_EntryOpen = false;
    }
}

void CursorHistoryModel::GoTo(int index)
{
  const Entry &e = m_Entries[index];
  GenericImageData *gid = m_Parent->GetDriver()->GetCurrentImageData();

  // Move the cursor without recording the move
  m_Navigating = true;
  gid->SetCursorPositionRAS(e.ras);
  if(e.tp < gid->GetNumberOfTimePoints())
    gid->SetCursorTimePoint(e.tp);
  m_Navigating = false;

  m_Index = index;
  m_EntryOpen = false;
  this->InvokeEvent(StateMachineChangeEvent());
}

void CursorHistoryModel::GoBack()
{
  if(!this->CheckState(UIF_CAN_GO_BACK))
    return;

  // If the cursor moved without being recorded (e.g., during 4D replay),
  // record where it is now, so that going forward brings the user back here
  if(!IsSamePlace(GetCurrentPlace(), m_Entries[m_Index]))
    AddCurrentPlace();

  GoTo(m_Index - 1);
}

void CursorHistoryModel::GoForward()
{
  if(this->CheckState(UIF_CAN_GO_FORWARD))
    GoTo(m_Index + 1);
}

void CursorHistoryModel::Reset()
{
  if(!this->CheckState(UIF_CAN_RESET))
    return;

  // The center is always a new entry, so that the user can go back from it
  m_EntryOpen = false;
  m_Parent->GetDriver()->ResetCursorPosition();
  m_EntryOpen = false;
}
