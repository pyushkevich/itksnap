#include "SaveModifiedLayersModel.h"
#include "ImageWrapperBase.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "IRISImageData.h"
#include "ImageIODelegates.h"


bool AbstractSaveableItem::IsSaveable()
{
  for(DependencyList::iterator it = m_Dependencies.begin(); it != m_Dependencies.end(); ++it)
    if((*it)->NeedsDecision())
      return false;
  return true;
}

void AbstractSaveableItem::AddDependency(AbstractSaveableItem *dep)
{
  m_Dependencies.push_back(dep);
}





void ImageLayerSaveableItem
::Initialize(SaveModifiedLayersModel *model, ImageWrapperBase *wrapper, LayerRole role)
{
  m_Model = model;
  m_Wrapper = wrapper;
  m_Role = role;
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

bool ImageLayerSaveableItem::IsSaved()
{
  return !m_Wrapper->HasUnsavedChanges();
}

bool ImageLayerSaveableItem::Save(AbstractSaveModifiedLayersInteractionDelegate *cb_delegate)
{
  // Create a delegate for saving this image
  return cb_delegate->SaveImageLayer(m_Model->GetParentModel(), m_Wrapper, m_Role);
}

bool ImageLayerSaveableItem::RequiresInteraction()
{
  return GetFilename().size() == 0;
}





void WorkspaceSaveableItem::Initialize(SaveModifiedLayersModel *model)
{
  m_Model = model;
  m_Saved = false;
}

std::string WorkspaceSaveableItem::GetDescription() const
{
  return std::string("Workspace");
}

std::string WorkspaceSaveableItem::GetFilename() const
{
  return m_Model->GetParentModel()->GetGlobalState()->GetProjectFilename();
}

bool WorkspaceSaveableItem::IsSaved()
{
  return m_Saved;
}

bool WorkspaceSaveableItem::Save(AbstractSaveModifiedLayersInteractionDelegate *cb_delegate)
{
  m_Saved = cb_delegate->SaveProject(m_Model->GetParentModel());
  return m_Saved;
}

bool WorkspaceSaveableItem::RequiresInteraction()
{
  return this->GetFilename().size() == 0;
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

void SaveModifiedLayersModel::BuildUnsavedItemsList(std::list<ImageWrapperBase *> layers)
{
  // Clear the list
  m_UnsavedItems.clear();

  // Get the list of all the layers that are applicable. Notice that we only
  // consider the IRIS image data, we don't support saving of layers from
  // SNAP mode. This might change in the future.
  GenericImageData *gid = m_ParentModel->GetDriver()->GetIRISImageData();

  // Are there any layers without filenames
  bool flag_unnamed_layers = false;

  // Is the main image layer included (i.e., the workspace is being closed)
  bool flag_main_included = false;

  // For each layer, check if it needs saving
  // These are the types of layers that we consider "worth saving".
  // TODO: this should be a parameter of the class, probably
  for(LayerIterator lit = gid->GetLayers(MAIN_ROLE | LABEL_ROLE | OVERLAY_ROLE);
      !lit.IsAtEnd(); ++lit)
    {
    ImageWrapperBase *w = lit.GetLayer();

    if(layers.size() == 0 || std::find(layers.begin(), layers.end(), w) != layers.end())
      {
      // Is the main layer being included in what's discarded?
      if(w == gid->GetMain())
        flag_main_included = true;

      if(w->HasUnsavedChanges())
        {
        // Add a new item
        SmartPtr<ImageLayerSaveableItem> item = ImageLayerSaveableItem::New();
        item->Initialize(this, w, lit.GetRole());
        item->SetId(m_UnsavedItems.size());
        m_UnsavedItems.push_back(item.GetPointer());

        if(item->RequiresInteraction())
          flag_unnamed_layers = true;
        }
      }
    }

  // The workspace should only be included in the list if the main image is being
  // unloaded. Otherwise, any changes (loading/unloading) should be considered as
  // part of editing the workspace, and thus we should not be prompting to save the
  // workspace. Also, the workspace must already exist.
  if(flag_main_included && m_ParentModel->GetGlobalState()->GetProjectFilename().size())
    {
    // Additional conditions are that either the project has unsaved changes, or one of
    // the items proposed for saving does not have a filename yet (and so saving it would
    // cause a modification to the workspace)
    if(flag_unnamed_layers | m_ParentModel->GetDriver()->IsProjectUnsaved())
      {
      SmartPtr<WorkspaceSaveableItem> item = WorkspaceSaveableItem::New();
      item->Initialize(this);
      item->SetId(m_UnsavedItems.size());

      // Add all of the other unsaved items as dependencies
      for(int i = 0; i < m_UnsavedItems.size(); i++)
        item->AddDependency(m_UnsavedItems[i]);

      m_UnsavedItems.push_back(item.GetPointer());
      }

    }

  // Adjust the current item
  m_CurrentItem = 0;
}

void SaveModifiedLayersModel::Initialize(GlobalUIModel *parent, std::list<ImageWrapperBase *> layers)
{
  m_ParentModel = parent;

  // The model is not listening to changes in the layers because the dialog
  // it supports is meant to be modal, i.e. the user can't make changes.

  // Update the list of unsaved items
  BuildUnsavedItemsList(layers);
}

bool SaveModifiedLayersModel::CheckState(SaveModifiedLayersModel::UIState state)
{
  int i;
  switch(state)
    {
    case SaveModifiedLayersModel::UIF_CAN_SAVE_ALL:
      for(i = 0; i < m_UnsavedItems.size(); i++)
        if(m_UnsavedItems[i]->NeedsDecision() && m_UnsavedItems[i]->RequiresInteraction())
          return false;
      return true;

    case SaveModifiedLayersModel::UIF_CAN_SAVE_CURRENT:
      return (m_CurrentItem >= 0 && m_CurrentItem < m_UnsavedItems.size()
              && m_UnsavedItems[m_CurrentItem]->IsSaveable());

    case SaveModifiedLayersModel::UIF_CAN_DISCARD_CURRENT:
      return true;

    case SaveModifiedLayersModel::UIF_CAN_QUICKSAVE_CURRENT:
      break;

    }
  return false;
}

void SaveModifiedLayersModel::UpdateCurrentItem()
{
  for(int delta = 1; delta < m_UnsavedItems.size(); delta++)
    {
    int testItem = (m_CurrentItem + delta) % m_UnsavedItems.size();
    if(m_UnsavedItems[testItem]->NeedsDecision()
       && m_UnsavedItems[testItem]->IsSaveable())
      {
      SetCurrentItem(testItem);
      return;
      }
    }
}

void SaveModifiedLayersModel::SaveCurrent()
{
  assert(m_CurrentItem >= 0 && m_CurrentItem < m_UnsavedItems.size());

  // Try saving
  if(m_UnsavedItems[m_CurrentItem]->Save(m_UIDelegate))
    {
    // Change the current item to the next unsaved item
    this->UpdateCurrentItem();

    // Fire an update event
    this->InvokeEvent(ModelUpdateEvent());
    }
}

void SaveModifiedLayersModel::DiscardCurrent()
{
  assert(m_CurrentItem >= 0 && m_CurrentItem < m_UnsavedItems.size());

  // Discarding does not cause any action. It just means that the item should
  // no longer appear on the 'to save' list.
  m_UnsavedItems[m_CurrentItem]->SetDiscarded(true);

  // Change the current item to the next unsaved item
  this->UpdateCurrentItem();

  // Fire an update event
  this->InvokeEvent(ModelUpdateEvent());
}

void SaveModifiedLayersModel::SaveAll()
{
  // For all items that need decision, save them, respecting dependencies
  int n_Unsaved = m_UnsavedItems.size();
  while(n_Unsaved > 0)
    {
    n_Unsaved = 0;
    for(int i = 0; i < m_UnsavedItems.size(); i++)
      {
      if(m_UnsavedItems[i]->NeedsDecision())
        {
        if(m_UnsavedItems[i]->IsSaveable())
          m_UnsavedItems[i]->Save(m_UIDelegate);
        else
          n_Unsaved++;
        }
      }
    }

  // Fire an update event
  this->InvokeEvent(ModelUpdateEvent());
}
