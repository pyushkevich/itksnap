#ifndef LABELEDITORMODEL_H
#define LABELEDITORMODEL_H

#include <AbstractModel.h>
#include <PropertyModel.h>
#include <ColorLabel.h>
#include <ColorLabelTable.h>
#include <GlobalUIModel.h>

typedef ConcretePropertyModel<
  LabelType, STLMapItemSetDomain<LabelType, ColorLabel> > ConcreteColorLabelPropertyModelBase;


/**
  This is an ITK-SNAP model that internally stores a color label and provides
  a set of options, correponding to currently available color labels
  */
class ConcreteColorLabelPropertyModel : public ConcreteColorLabelPropertyModelBase
{
public:
  // Standard ITK stuff
  typedef STLMapItemSetDomain<LabelType, ColorLabel> DomainType;
  typedef ConcreteColorLabelPropertyModel Self;
  typedef ConcretePropertyModel<LabelType, DomainType> Superclass;
  typedef SmartPtr<Self> Pointer;
  typedef SmartPtr<const Self> ConstPointer;
  itkTypeMacro(ConcreteColorLabelPropertyModel, ConcretePropertyModel)
  itkNewMacro(Self)

  /** Set the color label table, from which this model constructs its domain
    representation */
  void Initialize(ColorLabelTable *clt)
  {
    // Ititialize the domain representation
    DomainType dom(&clt->GetValidLabels());
    this->SetDomain(dom);

    // We should also listen to events from the label table, and rebroadcast
    // as changes to the domain. Note that there are two types of changes to the
    // label table, one that is a reconfiguration and another that is a property
    // change. These map to different kinds of domain change events.
    Rebroadcast(clt, SegmentationLabelConfigurationChangeEvent(), DomainChangedEvent());
    Rebroadcast(clt, SegmentationLabelPropertyChangeEvent(), DomainDescriptionChangedEvent());
  }
};



class LabelEditorModel : public AbstractModel
{
public:
  typedef AbstractRangedPropertyModel<double>::Type DoubleValueModel;
  typedef AbstractRangedPropertyModel<int>::Type IntegerValueModel;
  typedef AbstractRangedPropertyModel<bool>::Type BooleanValueModel;
  typedef AbstractPropertyModel<std::string> StringValueModel;
  typedef AbstractPropertyModel<iris_vector_fixed<bool, 2> > BooleanPairValueModel;
  typedef AbstractPropertyModel<Vector3ui> RGBColorValueModel;

  // Standard ITK stuff
  irisITKObjectMacro(LabelEditorModel, AbstractModel)

  /** Initialize with the parent model */
  void SetParentModel(GlobalUIModel *parent);

  /** Get the model describing the current selected label (and its domain) */
  irisGetMacro(CurrentLabelModel, ConcreteColorLabelPropertyModel *)

  /** Get the model for the current label description */
  irisGetMacro(CurrentLabelDescriptionModel, StringValueModel *)

  /** Get the model for the current label id */
  irisGetMacro(CurrentLabelIdModel, IntegerValueModel *)

  /** Get the model for the current label id */
  irisGetMacro(CurrentLabelOpacityModel, IntegerValueModel *)

  /** Get the model for the current label id */
  irisGetMacro(CurrentLabelHiddenStateModel, BooleanPairValueModel *)

  /** Get the model for the current label id */
  irisGetMacro(CurrentLabelColorModel, RGBColorValueModel *)

  protected:

  // Hidden constructor/destructor
  LabelEditorModel();
  virtual ~LabelEditorModel() {}

  // Helper method to retrieve current color label from the table
  bool GetAndStoreCurrentLabel();

  // Id of the current label
  SmartPtr<IntegerValueModel> m_CurrentLabelIdModel;
  bool GetCurrentLabelIdValueAndRange(
      int &value, NumericValueRange<int> *domain);
  void SetCurrentLabelId(int value);

  // Opacity of the current label
  SmartPtr<IntegerValueModel> m_CurrentLabelOpacityModel;
  bool GetCurrentLabelOpacityValueAndRange(
      int &value, NumericValueRange<int> *domain);
  void SetCurrentLabelOpacity(int value);

  // Description of the current label
  SmartPtr<StringValueModel> m_CurrentLabelDescriptionModel;
  bool GetCurrentLabelDescription(std::string &value);
  void SetCurrentLabelDescription(std::string value);

  // Visibility of the current label
  SmartPtr<BooleanPairValueModel> m_CurrentLabelHiddenStateModel;
  bool GetCurrentLabelHiddenState(iris_vector_fixed<bool, 2> &value);
  void SetCurrentLabelHiddenState(iris_vector_fixed<bool, 2> value);

  // Color of the current label
  SmartPtr<RGBColorValueModel> m_CurrentLabelColorModel;
  bool GetCurrentLabelColor(Vector3ui &value);
  void SetCurrentLabelColor(Vector3ui value);

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
