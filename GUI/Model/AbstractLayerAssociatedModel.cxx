#include "AbstractLayerAssociatedModel.h"


template<class TProperties, class TWrapper>
void
AbstractLayerAssociatedModel<TProperties,TWrapper>
::SetLayer(TWrapper *layer)
{
  // Make sure the layer-specific stuff is up to date
  m_LayerProperties.Update();

  // Unregister from the current layer
  if(m_LayerProperties.find(m_Layer) != m_LayerProperties.end())
    this->UnRegisterFromLayer(m_Layer);

  // Set the layer
  m_Layer = layer;

  // Handle events. Need to be careful here, because layers are dynamically
  // changing, and we don't want to add more than one observer to any layer.
  // Note that we don't remove the observer from the old layer because when
  // this method is called, the old layer may have already been destroyed!
  if(m_Layer)
    this->RegisterWithLayer(m_Layer);

  // Fire an event to indicate the change
  InvokeEvent(ModelUpdateEvent());
}

template<class TProperties, class TWrapper>
AbstractLayerAssociatedModel<TProperties,TWrapper>::TProperties &
AbstractLayerAssociatedModel<TProperties,TWrapper>::GetProperties()
{
  assert(m_Layer);
  return *m_LayerProperties[m_Layer];
}
