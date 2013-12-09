#ifndef SAVEMODIFIEDLAYERSMODEL_H
#define SAVEMODIFIEDLAYERSMODEL_H

#include <PropertyModel.h>

class ImageWrapperBase;
class GlobalUIModel;

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

  virtual bool Equals(const AbstractSaveableItem *other) const = 0;

  irisGetSetMacro(Id, int)

protected:

  int m_Id;
};

/**
 * A saveable item representing an image layer
 */
class ImageLayerSaveableItem : public AbstractSaveableItem
{
public:

  irisITKObjectMacro(ImageLayerSaveableItem, AbstractSaveableItem)

  void SetWrapper(ImageWrapperBase *wrapper);

  virtual std::string GetDescription() const;
  virtual std::string GetFilename() const;
  virtual bool Equals(const AbstractSaveableItem *other) const;

protected:

  // The image wrapper
  ImageWrapperBase *m_Wrapper;

};


/**
 * This model supports saving or discarding of image layers. It is used when
 * unloading images and segmentations, quitting, etc.
 */
class SaveModifiedLayersModel : public AbstractModel
{
public:

  irisITKObjectMacro(SaveModifiedLayersModel, AbstractModel)

  void SetParentModel(GlobalUIModel *parent);

  enum UIState
  {
    UIF_CAN_SAVE_ALL = 0,
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

  virtual void OnUpdate();

protected:
  SaveModifiedLayersModel();

  // Parent model
  GlobalUIModel *m_ParentModel;

  // List of unsaved items
  SaveableItemPtrArray m_UnsavedItems;

  // Currently selected unsaved item
  int m_CurrentItem;

  SmartPtr<AbstractSimpleIntProperty> m_CurrentItemModel;
  bool GetCurrentItemValue(int &out_value);
  void SetCurrentItemValue(int value);

  // Update the list of unsaved items
  void UpdateUnsavedItemsList();
};

#endif // SAVEMODIFIEDLAYERSMODEL_H
