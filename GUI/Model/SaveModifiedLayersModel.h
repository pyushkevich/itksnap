#ifndef SAVEMODIFIEDLAYERSMODEL_H
#define SAVEMODIFIEDLAYERSMODEL_H

#include "PropertyModel.h"

class ImageWrapperBase;
class GlobalUIModel;
class ImageIOWizardModel;
class SaveModifiedLayersModel;

/**
 * In this dialog, the interaction between the model and the GUI class is a
 * bit complicated. Because some of the files that need to be saved may be
 * without a filename, there is the need to prompt the user for the filename
 * during the save operation. The easiest way to do so is via a callback. So
 * the GUI has to provide a pointer to a child of this abstract delegate class
 */
class AbstractSaveModifiedLayersInteractionDelegate
{
public:
  virtual bool SaveImageLayer(
      GlobalUIModel *model, ImageWrapperBase *wrapper, LayerRole role) = 0;

  virtual bool SaveProject(GlobalUIModel *model) = 0;

};



/**
 * An abstract item that we might want to save. We inherit from AbstractModel
 * in order to use the smart pointer support, etc.
 */
class AbstractSaveableItem : public AbstractModel
{
public:
  irisITKAbstractObjectMacro(AbstractSaveableItem, AbstractModel)

  virtual std::string GetDescription() const = 0;
  virtual std::string GetFilename() const = 0;

  irisGetSetMacro(Id, int)

  /** Mark item as discarded */
  irisGetSetMacro(Discarded, bool)

  /** Save the item (pass in the delegate to allow call-back to GUI */
  virtual bool Save(AbstractSaveModifiedLayersInteractionDelegate *delegate) = 0;

  /** Whether this item requires interaction to be saved */
  virtual bool RequiresInteraction() = 0;

  /** Whether this item needs decision from the user */
  bool NeedsDecision() { return !(m_Discarded || IsSaved()); }

  /** Whether this item can be saved. This is true if all dependencies have
   * been saved or discarded */
  virtual bool IsSaveable();

  // Add a dependency (the item is not saveable until all dependencies have
  // been saved). Be careful not to create circular dependencies!
  void AddDependency(AbstractSaveableItem *dep);

protected:

  AbstractSaveableItem() : m_Id(-1), m_Discarded(false) {}

  /** Whether the user needs to decide about this item */
  virtual bool IsSaved() = 0;

  int m_Id;
  bool m_Discarded;

  typedef std::list<AbstractSaveableItem *> DependencyList;
  DependencyList m_Dependencies;
};

/**
 * A saveable item representing an image layer
 */
class ImageLayerSaveableItem : public AbstractSaveableItem
{
public:

  irisITKObjectMacro(ImageLayerSaveableItem, AbstractSaveableItem)

  void Initialize(SaveModifiedLayersModel *model, ImageWrapperBase *wrapper, LayerRole role);

  virtual std::string GetDescription() const;
  virtual std::string GetFilename() const;

  virtual bool IsSaved();
  virtual bool Save(AbstractSaveModifiedLayersInteractionDelegate *cb_delegate);

  /** Whether this item requires interaction to be saved */
  virtual bool RequiresInteraction();

protected:

  // The image wrapper
  ImageWrapperBase *m_Wrapper;
  LayerRole m_Role;
  SaveModifiedLayersModel *m_Model;
};

/**
 * A saveable item representing a workspace state (aka project)
 */
class WorkspaceSaveableItem : public AbstractSaveableItem
{
public:

  irisITKObjectMacro(WorkspaceSaveableItem, AbstractSaveableItem)

  void Initialize(SaveModifiedLayersModel *model);

  virtual std::string GetDescription() const;
  virtual std::string GetFilename() const;

  virtual bool IsSaved();
  virtual bool Save(AbstractSaveModifiedLayersInteractionDelegate *cb_delegate);

  /** Whether this item requires interaction to be saved */
  virtual bool RequiresInteraction();

protected:

  // The image wrapper
  SaveModifiedLayersModel *m_Model;

  // Saved state - based on the return code from save method
  bool m_Saved;
};



/**
 * This model supports saving or discarding of image layers. It is used when
 * unloading images and segmentations, quitting, etc.
 */
class SaveModifiedLayersModel : public AbstractModel
{
public:

  irisITKObjectMacro(SaveModifiedLayersModel, AbstractModel)

  void Initialize(GlobalUIModel *parent, std::list<ImageWrapperBase *> layers);

  irisGetMacro(ParentModel, GlobalUIModel *)

  irisGetSetMacro(UIDelegate, AbstractSaveModifiedLayersInteractionDelegate *)

  enum UIState
  {
    UIF_CAN_SAVE_ALL = 0,
    UIF_CAN_SAVE_CURRENT,
    UIF_CAN_DISCARD_CURRENT,
    UIF_CAN_QUICKSAVE_CURRENT
  };

  bool CheckState(UIState state);

  irisSimplePropertyAccessMacro(CurrentItem, int)

  // Unsaved item definition
  typedef SmartPtr<AbstractSaveableItem> SaveableItemPtr;
  typedef std::vector<SaveableItemPtr> SaveableItemPtrArray;

  // Get the unsaved items
  irisGetMacro(UnsavedItems, const SaveableItemPtrArray &)

  // Save the current item (interactively or non-interactively)
  void SaveCurrent();

  // Discard the current item
  virtual void DiscardCurrent();

  // Save all items that need saving
  virtual void SaveAll();

protected:
  SaveModifiedLayersModel();

  // Parent model
  GlobalUIModel *m_ParentModel;

  // Delegate that allows callbacks to the GUI
  AbstractSaveModifiedLayersInteractionDelegate *m_UIDelegate;

  // List of unsaved items
  SaveableItemPtrArray m_UnsavedItems;

  // Currently selected unsaved item
  int m_CurrentItem;

  SmartPtr<AbstractSimpleIntProperty> m_CurrentItemModel;
  bool GetCurrentItemValue(int &out_value);
  void SetCurrentItemValue(int value);

  // Update the list of unsaved items
  void BuildUnsavedItemsList(std::list<ImageWrapperBase *> layers);

  // Update the current imate
  void UpdateCurrentItem();
};

#endif // SAVEMODIFIEDLAYERSMODEL_H
