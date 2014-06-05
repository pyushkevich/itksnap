#include "ColorLabelQuickListModel.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "ColorLabelTable.h"
#include <queue>

ColorLabelQuickListModel::ColorLabelQuickListModel()
{
  m_ActiveComboModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetActiveComboValueAndRange,
        &Self::SetActiveComboValue);
}

ColorLabelQuickListModel::~ColorLabelQuickListModel()
{
}

/*
void ColorLabelQuickListModel::ComputeRecent()
{
  // Get the color label table
  ColorLabelTable *clt = m_Parent->GetDriver()->GetColorLabelTable();

  // We use a pair of label id, access time to represent each label
  typedef std::pair<int, int> LabelPair;

  // Build the priority queue for keeping track of the most recent
  typedef std::priority_queue<LabelPair> HeapType;
  HeapType heap;

  // Go through all the items in the label list, maintaining the smallest
  // items in the queue
  for(ColorLabelTable::ValidLabelConstIterator it = clt->begin();
      it != clt->end(); ++it)
    {
    // Encode the label as (timestamp, -id) for sorting
    LabelPair pair = std::make_pair(-it->second.GetTimeStamp(), (int) it->first);

    // If the queue is not full, push the pair
    if(heap.size() < m_MaxStoredLabels)
      {
      heap.push(pair);
      }
    else if(heap.top() < pair)
      {
      heap.pop();
      heap.push(pair);
      }
    }

  // Now the list has the most recent labels
  unsigned int nItems = std::min(m_MaxStoredLabels, (unsigned int) heap.size());
  m_RecentLabels.resize(nItems);
  for(int i = 1; i <= nItems; i++)
    {
    m_RecentLabels[nItems-i] = heap.top().second;
    heap.pop();
    }

  // Now we sort the list by label id. The effect of this is that the order
  // of the labels in the list does not change as the user picks labels from
  // the quick list
  std::sort(m_RecentLabels.begin(), m_RecentLabels.end());
}
*/

void ColorLabelQuickListModel::SetParentModel(GlobalUIModel *parent)
{
  m_Parent = parent;
  m_LabelHistory = m_Parent->GetDriver()->GetLabelUseHistory();

  // Rebroadcast the segmentation and color label update events as
  // update events from this model
  Rebroadcast(m_Parent->GetDriver()->GetColorLabelTable(),
              SegmentationLabelConfigurationChangeEvent(), ModelUpdateEvent());
  Rebroadcast(m_LabelHistory, itk::ModifiedEvent(), ModelUpdateEvent());

  // The active label model needs to rebroadcast events from the active drawing
  // label in GlobalState
  m_ActiveComboModel->RebroadcastFromSourceProperty(
        m_Parent->GetDriver()->GetGlobalState()->GetDrawingColorLabelModel());
  m_ActiveComboModel->RebroadcastFromSourceProperty(
        m_Parent->GetDriver()->GetGlobalState()->GetDrawOverFilterModel());
}

void ColorLabelQuickListModel::OnUpdate()
{
  if(!m_EventBucket->IsEmpty())
    {
    // Update the table of recent labels
    m_RecentCombos.clear();
    int nItems = m_LabelHistory->GetSize();
    for(int i = 0; i < nItems; i++)
      m_RecentCombos.push_back(m_LabelHistory->GetHistoryEntry(i));

    // The last item is always the clear label
    m_RecentCombos.push_back(std::make_pair(0, DrawOverFilter()));
    }
}


bool ColorLabelQuickListModel::GetActiveComboValueAndRange(int &value)
{
  // Record the current state
  LabelCombo state = std::make_pair(
        m_Parent->GetGlobalState()->GetDrawingColorLabel(),
        m_Parent->GetGlobalState()->GetDrawOverFilter());

  // See if any of the active labels matches the current state
  for(int i = 0; i < m_RecentCombos.size(); i++)
    {
    if(m_RecentCombos[i] == state)
      {
      value = i;
      return true;
      }
    }

  // Nothing matched, return false
  return false;
}

void ColorLabelQuickListModel::SetActiveComboValue(int value)
{
  assert(value < m_RecentCombos.size());
  LabelCombo state = m_RecentCombos[value];

  m_Parent->GetGlobalState()->SetDrawingColorLabel(state.first);
  m_Parent->GetGlobalState()->SetDrawOverFilter(state.second);
}
