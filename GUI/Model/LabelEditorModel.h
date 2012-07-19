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
    // as changes to the domain
    // TODO: this->Rebroadcast(clt, ???, RangeChangedEvent());
  }
};



class LabelEditorModel : public AbstractModel
{
public:
  // Standard ITK stuff
  irisITKObjectMacro(LabelEditorModel, AbstractModel)

  /** Initialize with the parent model */
  void SetParentModel(GlobalUIModel *parent);

  /** Get the model describing the current selected label (and its domain) */
  irisGetMacro(CurrentLabelModel, ConcreteColorLabelPropertyModel *)

protected:

  // Hidden constructor/destructor
  LabelEditorModel();
  virtual ~LabelEditorModel() {}

  // The parent model
  GlobalUIModel *m_Parent;

  // The label that is currently selected
  SmartPtr<ConcreteColorLabelPropertyModel> m_CurrentLabelModel;

};

#endif // LABELEDITORMODEL_H
