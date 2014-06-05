#ifndef COLORLABELQUICKLISTMODEL_H
#define COLORLABELQUICKLISTMODEL_H

#include <PropertyModel.h>
#include <LabelUseHistory.h>

class GlobalUIModel;

/**
 * This model provides a list of the recently used color labels in ITK-SNAP.
 * It can be used to create quick palettes of labels.
 */
class ColorLabelQuickListModel : public AbstractModel
{
public:

  irisITKObjectMacro(ColorLabelQuickListModel, AbstractModel)

  // Foreground/background label combination
  typedef LabelUseHistory::Entry LabelCombo;
  typedef std::vector<LabelCombo> ComboList;

  /** Assign a parent model to this model */
  void SetParentModel(GlobalUIModel *parent);

  /** Get the list of combos to include */
  irisGetMacro(RecentCombos, const ComboList &)

  /** This model describes the active label combination in the quick list */
  irisSimplePropertyAccessMacro(ActiveCombo, int)

protected:

  // The parent model
  GlobalUIModel *m_Parent;

  // The label history object
  LabelUseHistory *m_LabelHistory;

  // Cached list of active combos
  ComboList m_RecentCombos;

  // Model for the active label
  SmartPtr<AbstractSimpleIntProperty> m_ActiveComboModel;

  bool GetActiveComboValueAndRange(int &value);
  void SetActiveComboValue(int value);

  ColorLabelQuickListModel();
  ~ColorLabelQuickListModel();

  // Respond to an update in the model
  virtual void OnUpdate();

  // Compute the recent labels
  void ComputeRecent();
};

#endif // COLORLABELQUICKLISTMODEL_H
