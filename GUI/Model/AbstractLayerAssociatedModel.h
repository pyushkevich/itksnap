#ifndef ABSTRACTLAYERASSOCIATEDMODEL_H
#define ABSTRACTLAYERASSOCIATEDMODEL_H

#include "AbstractModel.h"
#include "LayerAssociation.h"
#include "GlobalUIModel.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "SNAPEventListenerCallbacks.h"
#include "ImageWrapperBase.h"
#include "PropertyModel.h"

/**
  This is an abstract class for a special type of UI model that can be
  associated with different image layers in SNAP. Examples of these models
  are models for contrast adjustment, colormap adjustment, etc., i.e.,
  models that link the GUI with one image layer. Two options were available:
  to associate a single such model with each image layer, or to create just
  one model, and allow the layer for that model to be switched. I chose
  the second option because it reduced the number of models that have to be
  kept around. However, the model has to keep track of layer-specific
  properties, and also has to be smart about registering and unregistering
  for events originating from the layers.

  This class is templated over a properties object, which contains some
  layer-specific properties that the model needs to store. For example, it
  can be the number of histogram bins to display for the contrast dialog.

  The second parameter is the type of the image wrapper that can participate
  in the association. It can be ImageWrapperBase or one of its subclasses.
*/
template <class TProperties, class TWrapper = ImageWrapperBase>
class AbstractLayerAssociatedModel : public AbstractModel
{
public:

  typedef AbstractLayerAssociatedModel<TProperties,TWrapper> Self;
  typedef AbstractModel Superclass;
  typedef SmartPtr<Self> Pointer;
  typedef SmartPtr<const Self> ConstPointer;
  itkTypeMacro(AbstractLayerAssociatedModel, AbstractModel)

  // An event fired when the selected layer changes
  itkEventMacro(ActiveLayerChangedEvent, ModelUpdateEvent)
  itkEventMacro(LayerStructureChangedEvent, ModelUpdateEvent)

  FIRES(LayerStructureChangedEvent)
  FIRES(ActiveLayerChangedEvent)

  irisGetMacro(ParentModel, GlobalUIModel *)
  void SetParentModel(GlobalUIModel *parent)
  {
    // Store the parent model
    m_ParentModel = parent;

    // Associate the layers with properties.
    m_LayerProperties.SetSource(m_ParentModel->GetDriver());

    // Layer changes in the parent are rebroadcast as model updates
    Rebroadcast(m_ParentModel, LayerChangeEvent(), LayerStructureChangedEvent());

    // Set active layer to NULL
    this->SetLayer(NULL);
  }



  /**
    Set the layer with which the model is associated. This can be NULL,
    in which case, the model will be dissasociated from all layers.
    */
  void SetLayer(TWrapper *layer)
  {
    // There is nothing to do if the layer is already set
    if(layer && m_Layer == layer)
      return;

    // Make sure the layer-specific stuff is up to date
    m_LayerProperties.Update();

    // Unregister from the current layer
    if(m_LayerProperties.HasLayer(m_Layer))
      {
      // Remove the deletion listener
      m_Layer->RemoveObserver(m_DeleteEventObserverTag);

      // Call the child's unregister method
      this->UnRegisterFromLayer(m_Layer, false);
      }

    // Set the layer
    m_Layer = layer;

    // Handle events. Need to be careful here, because layers are dynamically
    // changing, and we don't want to add more than one observer to any layer.
    // Note that we don't remove the observer from the old layer because when
    // this method is called, the old layer may have already been destroyed!
    if(m_Layer)
      {
      // Listen for delete events from the layer
      m_DeleteEventObserverTag =
          AddListener(m_Layer, itk::DeleteEvent(),
                      this, &Self::LayerDeletionCallback);

#ifdef SNAP_DEBUG_EVENTS
      if(flag_snap_debug_events)
        {
        std::cout << "DeleteEvent registration "
                  << " layer " << m_Layer << " id " << m_Layer->GetUniqueId()
                  << " observer " << this << std::endl << std::flush;
        }
#endif

      // Do whatever needs to be done to listen to layer events
      this->RegisterWithLayer(m_Layer);
      }

    // Fire an event to indicate the change
    InvokeEvent(ActiveLayerChangedEvent());
  }


  /** Get the layer associated with the model, or NULL if there is none */
  irisGetMacro(Layer, TWrapper *)

  /** Get the properties associated with the current layer */
  TProperties &GetProperties()
  {
    assert(m_Layer);
    TProperties *p = m_LayerProperties[m_Layer];
    return *p;
  }


  /**
    This method should be implemented by the child class. It registers
    the child class to rebroadcast whatever events it needs from the layer
    with which it has been associated to the downstream objects.
  */
  virtual void RegisterWithLayer(TWrapper *layer) = 0;


  /**
    This method should be implemented by the child class. It disconnects
    the chold class from the associated layer (just before breaking the
    association). For the Register/Unregister pair to work, the Register
    method implementation should retain the tag returned by the call to
    the Rebroadcast method. This tag can be placed in the layer-associated
    properties, and then used during the call to UnRegister.

    The second parameter to this method specifies whether the method is
    called in response to the layer being deleted or not. If they layer is
    being deleted, we are unsure about the state of the layer and we don't
    need to remove observers from it.
. */
  virtual void UnRegisterFromLayer(TWrapper *layer, bool being_deleted) = 0;

  /**
    The model has its own OnUpdate implementation, which handles changes
    in the layer structure. If the event bucket has a LayerChangeEvent,
    the model will automatically rebuild it's layer associations, and
    may reset the current layer to NULL if the current layer has been
    removed.

    If child models reimplement OnUpdate(), they must call
    AbstractLayerAssociatedModel::OnUpdate() within the reimplemented method.
    */
  virtual void OnUpdate()
  {
    if(m_EventBucket->HasEvent(LayerChangeEvent()))
      {
      // If the layers have changed, we need to update the layer properties
      // object. Then we need to see if the current layer has actually been
      // destroyed
      m_LayerProperties.Update();

      // Was the current layer removed?
      if(!m_LayerProperties.HasLayer(m_Layer))
        this->SetLayer(NULL);
      }
  }

  /**
   * A boolean property model indicating whether the model is holding a
   * layer or not. This can be used to toggle parts of the user interface
   */
  irisGetMacro(HasLayerModel, AbstractSimpleBooleanProperty * )


protected:
  AbstractLayerAssociatedModel()
  {
    // Set up the factory
    PropertiesFactory factory;
    factory.m_Model = this;
    m_LayerProperties.SetDelegate(factory);

    m_ParentModel = NULL;
    m_Layer = NULL;

    m_HasLayerModel = wrapGetterSetterPairAsProperty(
          this, &Self::GetHasLayerValue);
  }

  virtual ~AbstractLayerAssociatedModel() {}

  /** Create a  property object for a new layer */
  TProperties *CreateProperty(TWrapper *w)
  {
    return new TProperties();
  }

  void LayerDeletionCallback()
  {
    // Unregister from the current layer
    this->UnRegisterFromLayer(m_Layer, true);

    // Set the layer to NULL
    m_Layer = NULL;

    // Fire an event to indicate the change
    InvokeEvent(ActiveLayerChangedEvent());
  }

  // Parent model
  GlobalUIModel *m_ParentModel;

  // Currently associated layer
  TWrapper *m_Layer;

  // Tag used to manage deletion observers on the current layer
  unsigned long m_DeleteEventObserverTag;

  // A factory class for creating properties
  class PropertiesFactory
  {
  public:
    TProperties *New(TWrapper *w)
      { return m_Model->CreateProperty(w); }
    Self *m_Model;
  };

  // Model as to whether the layer is set
  SmartPtr<AbstractSimpleBooleanProperty> m_HasLayerModel;
  bool GetHasLayerValue()
  {
    return this->m_Layer != NULL;
  }

  // Association between a layer and a set of properties
  typedef LayerAssociation<
    TProperties,TWrapper,PropertiesFactory> LayerMapType;

  LayerMapType m_LayerProperties;

};

#endif // ABSTRACTLAYERASSOCIATEDMODEL_H
