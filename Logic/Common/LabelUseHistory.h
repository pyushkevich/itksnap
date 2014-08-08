#ifndef LABELUSEHISTORY_H
#define LABELUSEHISTORY_H

#include <utility>
#include <vector>
#include "SNAPCommon.h"
#include <itkDataObject.h>
#include <itkObjectFactory.h>

class ColorLabelTable;


/**
 * This class keeps track of the labels used during segmentation. It should
 * be updated whenever a new set of labels is used, and cleared when a new
 * label set is loaded.
 */
class LabelUseHistory : public itk::DataObject
{
public:
  irisITKObjectMacro(LabelUseHistory, DataObject)

  /** Type describing a foreground/background combination */
  typedef std::pair<LabelType, DrawOverFilter> Entry;

  /** Initialize to refer to a label table */
  void SetColorLabelTable(ColorLabelTable *clt);

  /** Record that a pair of labels has been used, i.e., to update segmentation */
  void RecordLabelUse(LabelType fore, DrawOverFilter back);

  /** Reset completely (initialize to use the first K labels) */
  void Reset();

  /** Get the number of labels in history (limited by max) */
  int GetSize();

  /** Get the maximum number of entries (hard-coded) */
  static int GetMaximumSize() { return 6; }

  /** Get the n-th entry */
  const Entry &GetHistoryEntry(int i);

protected:

  LabelUseHistory();
  ~LabelUseHistory() {}

  // Data structure keeping track of the labels
  typedef std::pair<Entry, unsigned long> EntryRecord;

  // The list of records
  typedef std::vector<EntryRecord> EntryRecordList;
  typedef EntryRecordList::iterator EntryRecordIter;
  typedef EntryRecordList::const_iterator EntryRecordCIter;

  // The actual list
  EntryRecordList m_History;

  // Pointer to the color label table
  ColorLabelTable *m_ColorLabelTable;

  // Flag to prevent infinite recursion
  bool m_ReconfigureActive;

  // A timestamp counter - updated each time RecordLabelUse is called
  unsigned long m_Counter;

  /** When the label table changes (labels added, deleted), update */
  void OnLabelTableReconfiguration();
};

#endif // LABELUSEHISTORY_H
