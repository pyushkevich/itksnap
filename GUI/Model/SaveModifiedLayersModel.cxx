#include "SaveModifiedLayersModel.h"
#include "ImageWrapperBase.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "IRISImageData.h"

void ImageLayerSaveableItem::SetWrapper(ImageWrapperBase *wrapper)
{
  m_Wrapper = wrapper;
}

std::string ImageLayerSaveableItem::GetDescription() const
{
  if(m_Wrapper->GetCustomNickname().length())
    return m_Wrapper->GetCustomNickname();
  else
    return m_Wrapper->GetDefaultNickname();
}

std::string ImageLayerSaveableItem::GetFilename() const
{
  return m_Wrapper->GetFileName();
}

bool ImageLayerSaveableItem::Equals(const AbstractSaveableItem *other) const
{
  const Self *other_cast = dynamic_cast<const Self *>(other);
  return(other_cast && other_cast->m_Wrapper == m_Wrapper);
}

SaveModifiedLayersModel::SaveModifiedLayersModel()
{
  m_CurrentItemModel = wrapGetterSetterPairAsProperty(
        this, &Self::GetCurrentItemValue, &Self::SetCurrentItemValue);

  m_CurrentItem = 0;
}

bool SaveModifiedLayersModel::GetCurrentItemValue(int &out_value)
{
  out_value = m_CurrentItem;
  return true;
}

void SaveModifiedLayersModel::SetCurrentItemValue(int value)
{
  m_CurrentItem = value;

  // Do something?
  InvokeEvent(StateMachineChangeEvent());
}

void SaveModifiedLayersModel::UpdateUnsavedItemsList()
{
  // See what the current item is, because we want to preserve it
  SmartPtr<AbstractSaveableItem> currentItemPtr =
      (m_CurrentItem >= 0 && m_CurrentItem < m_UnsavedItems.size())
      ? m_UnsavedItems[m_CurrentItem] : NULL;

  // Clear the list
  m_UnsavedItems.clear();

  // Get the list of all the layers that are applicable. Notice that we only
  // consider the IRIS image data, we don't support saving of layers from
  // SNAP mode. This might change in the future.
  GenericImageData *gid = m_ParentModel->GetDriver()->GetIRISImageData();

  // These are the types of layers that we consider "worth saving".
  // TODO: this should be a parameter of the class, probably
  LayerIterator lit = gid->GetLayers(MAIN_ROLE | LABEL_ROLE | OVERLAY_ROLE);

  // For each layer, check if it needs saving
  while(!lit.IsAtEnd())
    {
    ImageWrapperBase *w = lit.GetLayer();

    if(w->HasUnsavedChanges())
      {
      // Add a new item
      SmartPtr<ImageLayerSaveableItem> item = ImageLayerSaveableItem::New();
      item->SetWrapper(w);
      item->SetId(m_UnsavedItems.size());
      m_UnsavedItems.push_back(item.GetPointer());

      // See if this item needs to be selected
      if(item->Equals(currentItemPtr))
        m_CurrentItem = item->GetId();
      }

    ++lit;
    }

  // Adjust the current item
  if(m_CurrentItem >= m_UnsavedItems.size())
    m_CurrentItem = 0;
}

void SaveModifiedLayersModel::SetParentModel(GlobalUIModel *parent)
{
  m_ParentModel = parent;

  // Listen for events that affect what's unsaved
  Rebroadcast(m_ParentModel->GetDriver(), LayerChangeEvent(), ModelUpdateEvent());
  Rebroadcast(m_ParentModel->GetDriver(), WrapperMetadataChangeEvent(), ModelUpdateEvent());

  // Update the list of unsaved items
  UpdateUnsavedItemsList();
}

bool SaveModifiedLayersModel::CheckState(SaveModifiedLayersModel::UIState state)
{
  switch(state)
    {
    case SaveModifiedLayersModel::UIF_CAN_SAVE_ALL:
      break;
    case SaveModifiedLayersModel::UIF_CAN_DISCARD_CURRENT:
      break;
    case SaveModifiedLayersModel::UIF_CAN_QUICKSAVE_CURRENT:
      break;

    }
  return false;
}

void SaveModifiedLayersModel::OnUpdate()
{
  if(this->m_EventBucket->HasEvent(LayerChangeEvent()) ||
     this->m_EventBucket->HasEvent(WrapperMetadataChangeEvent()))
    {
    // Update the unsaved items
    this->UpdateUnsavedItemsList();
    }
}
