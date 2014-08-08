#include "LabelUseHistory.h"
#include "ColorLabelTable.h"
#include <algorithm>

LabelUseHistory::LabelUseHistory()
{
  m_History.reserve(this->GetMaximumSize());
  m_Counter = 0;
  m_ReconfigureActive = false;
}

void LabelUseHistory::SetColorLabelTable(ColorLabelTable *clt)
{
  m_ColorLabelTable = clt;
  this->Reset();
}

void LabelUseHistory::RecordLabelUse(LabelType fore, DrawOverFilter back)
{
  // We always want to make sure we are ahead of the label table
  if(m_ColorLabelTable->GetTimeStamp() > this->GetTimeStamp()
     && !m_ReconfigureActive)
    this->OnLabelTableReconfiguration();

  // We should ignore the clear label / draw over all
  if(fore == 0 && back.CoverageMode == PAINT_OVER_ALL)
    return;

  // Create an entry
  Entry p = std::make_pair(fore, back);

  // See if the label is already contained in the table. If it is, we just
  // update the timestamp on the label in the history, but don't change the
  // order of any of the entries.
  for(EntryRecordIter it = m_History.begin(); it != m_History.end(); ++it)
    {
    EntryRecord &rec = *it;
    if(rec.first == p)
      {
      rec.second = m_Counter++;
      return;
      }
    }

  // No, the new Entry was not in the history. In this case, we either append
  // the new entry (if there is room), or replace another entry with it

  // Create an entry record with a time stamp
  EntryRecord prec = std::make_pair(p, m_Counter++);

  if(m_History.size() < GetMaximumSize())
    {
    m_History.push_back(prec);
    }
  else
    {
    // Search for the oldest element
    int oldest = 0, pos_oldest = -1;
    for(int i = 0; i < m_History.size(); ++i)
      {
      if(oldest == 0 || m_History[i].second < oldest)
        {
        oldest = m_History[i].second;
        pos_oldest = i;
        }
      }

    // Replace the oldest element
    m_History[pos_oldest] = prec;
    }

  // As far as the world is concerned, we have been modified
  this->Modified();
}

void LabelUseHistory::OnLabelTableReconfiguration()
{
  // Avoid recursive calls between this method and RecordLabelUse();
  m_ReconfigureActive = true;

  // Go through all the labels in the history and make sure they still
  // exist. If they don't, delete them
  EntryRecordIter it = m_History.begin();
  while(it != m_History.end())
    {
    Entry p = (*it).first;
    if(!m_ColorLabelTable->IsColorLabelValid(p.first) ||
       !m_ColorLabelTable->IsColorLabelValid(p.second.DrawOverLabel))
      {
      it = m_History.erase(it);
      }
    else
      ++it;
    }

  // Now add labels until the list is full
  ColorLabelTable::ValidLabelConstIterator vlcit = m_ColorLabelTable->begin();
  while(m_History.size() < GetMaximumSize() && vlcit != m_ColorLabelTable->end())
    {
    RecordLabelUse(vlcit->first, DrawOverFilter());
    ++vlcit;
    }

  // Modified
  this->Modified();

  // Avoid recursive calls between this method and RecordLabelUse();
  m_ReconfigureActive = false;
}

void LabelUseHistory::Reset()
{
  // Clear history
  m_History.clear();

  // Add labels until the list is full
  ColorLabelTable::ValidLabelConstIterator vlcit = m_ColorLabelTable->begin();
  while(m_History.size() < GetMaximumSize() && vlcit != m_ColorLabelTable->end())
    {
    // We don't add the clear label to the history
    if(vlcit->first != 0)
      {
      Entry p = std::make_pair(vlcit->first, DrawOverFilter());
      m_History.push_back(std::make_pair(p, m_Counter++));
      }

    ++vlcit;
    }

  this->Modified();
}

int LabelUseHistory::GetSize()
{
  // We always want to make sure we are ahead of the label table
  if(m_ColorLabelTable->GetTimeStamp() > this->GetTimeStamp())
    this->OnLabelTableReconfiguration();

  return m_History.size();
}

const LabelUseHistory::Entry &LabelUseHistory::GetHistoryEntry(int i)
{
  // We always want to make sure we are ahead of the label table
  if(m_ColorLabelTable->GetTimeStamp() > this->GetTimeStamp())
    this->OnLabelTableReconfiguration();

  return m_History[i].first;
}




