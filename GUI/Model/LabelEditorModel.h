#ifndef LABELEDITORMODEL_H
#define LABELEDITORMODEL_H

#include <AbstractModel.h>
#include <PropertyModel.h>
#include <ColorLabel.h>
#include <ColorLabelTable.h>
#include <GlobalUIModel.h>
#include <ColorLabelPropertyModel.h>

class LabelEditorModel : public AbstractModel
{
public:

  // Standard ITK stuff
  irisITKObjectMacro(LabelEditorModel, AbstractModel)

  /** Initialize with the parent model */
  void SetParentModel(GlobalUIModel *parent);

  /** Get the model describing the current selected label (and its domain) */
  irisGetMacro(CurrentLabelModel, ConcreteColorLabelPropertyModel *)

  /** Get the model for the current label description */
  irisGetMacro(CurrentLabelDescriptionModel, AbstractSimpleStringProperty *)

  /** Get the model for the current label id */
  irisGetMacro(CurrentLabelIdModel, AbstractRangedIntProperty *)

  /** Get the model for the current label id */
  irisGetMacro(CurrentLabelOpacityModel, AbstractRangedIntProperty *)

  /** Get the model for the current label id */
  irisGetMacro(CurrentLabelHiddenStateModel, AbstractSimpleBooleanVec2Property *)

  /** Get the model for the current label id */
  irisGetMacro(CurrentLabelColorModel, AbstractSimpleUIntVec3Property *)

  /** Get the model that specifies whether the current label is background or
   * foreground label */
  irisGetMacro(IsForegroundBackgroundModel, AbstractSimpleBooleanVec2Property *)

  /** Change the label id of the selected label */
  bool ReassignLabelId(LabelType newid);

  /**
    States in which the model can be, which allow the activation and
    deactivation of various widgets in the interface
    */
  enum UIState {
    UIF_EDITABLE_LABEL_SELECTED
  };

  /**
    Check the state flags above
    */
  bool CheckState(UIState state);

  /** Create a new label */
  bool MakeNewLabel(bool copyCurrent);

  /** Will deleting the current label affect the segmentation? */
  bool IsLabelDeletionDestructive();

  /** Delete the selected label */
  void DeleteCurrentLabel();

  /** Reset labels to defaults */
  void ResetLabels();

protected:

  // Hidden constructor/destructor
  LabelEditorModel();
  virtual ~LabelEditorModel() {}

  // Helper method to retrieve current color label from the table
  bool GetAndStoreCurrentLabel();

  // Id of the current label
  SmartPtr<AbstractRangedIntProperty> m_CurrentLabelIdModel;
  bool GetCurrentLabelIdValueAndRange(
      int &value, NumericValueRange<int> *domain);
  void SetCurrentLabelId(int value);

  // Opacity of the current label
  SmartPtr<AbstractRangedIntProperty> m_CurrentLabelOpacityModel;
  bool GetCurrentLabelOpacityValueAndRange(
      int &value, NumericValueRange<int> *domain);
  void SetCurrentLabelOpacity(int value);

  // Description of the current label
  SmartPtr<AbstractSimpleStringProperty> m_CurrentLabelDescriptionModel;
  bool GetCurrentLabelDescription(std::string &value);
  void SetCurrentLabelDescription(std::string value);

  // Visibility of the current label
  SmartPtr<AbstractSimpleBooleanVec2Property> m_CurrentLabelHiddenStateModel;
  bool GetCurrentLabelHiddenState(iris_vector_fixed<bool, 2> &value);
  void SetCurrentLabelHiddenState(iris_vector_fixed<bool, 2> value);

  // Color of the current label
  SmartPtr<AbstractSimpleUIntVec3Property> m_CurrentLabelColorModel;
  bool GetCurrentLabelColor(Vector3ui &value);
  void SetCurrentLabelColor(Vector3ui value);

  // Foreground/background status
  SmartPtr<AbstractSimpleBooleanVec2Property> m_IsForegroundBackgroundModel;
  bool GetIsForegroundBackground(Vector2b &value);
  void SetIsForegroundBackground(Vector2b value);

  // The parent model
  GlobalUIModel *m_Parent;

  // A pointer to the color label table
  ColorLabelTable *m_LabelTable;

  // The label that is currently selected
  SmartPtr<ConcreteColorLabelPropertyModel> m_CurrentLabelModel;

  // The information about the current label (temporarily valid)
  ColorLabel m_SelectedColorLabel;
  LabelType m_SelectedId;
};

#endif // LABELEDITORMODEL_H
