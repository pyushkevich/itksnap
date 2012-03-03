#ifndef ABSTRACTLAYERASSOCIATEDMODEL_H
#define ABSTRACTLAYERASSOCIATEDMODEL_H

#include "AbstractModel.h"

class GlobalUIModel;
class ImageWrapperBase;

/**
  This is an abstract class for a special type of UI model that can be
  associated with different image layers in SNAP. Examples of these models
  are models for contrast adjustment, colormap adjustment, etc., i.e.,
  models that link the GUI with one image layer. Two options were available:
  to associate a single such model with each image layer, or to create just
  one model, and allow the layer for that model to be switched out. I chose
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
  irisITKObjectMacro(AbstractLayerAssociatedModel, AbstractModel)

  FIRES(ModelUpdateEvent)

  irisGetMacro(ParentModel, GlobalUIModel *)
  void SetParentModel(GlobalUIModel *parent);


  /**
    Set the layer with which the model is associated. This can be NULL,
    in which case, the model will be dissasociated from all layers.
    */
  void SetLayer(TWrapper *layer);

  /** Get the layer associated with the model, or NULL if there is none */
  irisGetMacro(Layer, TWrapper *)

  /** Get the properties associated with the current layer */
  TProperties &GetProperties();

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
. */
  virtual void UnRegisterFromLayer(TWrapper *layer) = 0;




protected:
  AbstractLayerAssociatedModel() : AbstractModel() {}
  virtual ~AbstractLayerAssociatedModel() {}

  /** Create a new property object for a new layer */
  TProperties *CreateProperty(TWrapper *w);

  // Parent model
  GlobalUIModel *m_ParentModel;

  // Currently associated layer
  ImageWrapperBase *m_Layer;

  // A factory class for creating properties
  class PropertiesFactory
  {
  public:
    TProperties *New(TWrapper *f)
      { return m_Model->CreateProperty(w); }
    Self *m_Model;
  };

  // Association between a layer and a set of properties
  typedef LayerAssociation<
    TProperties,TWrapper,PropertiesFactory> LayerMapType;

  LayerMapType *m_LayerProperties;

};

#endif // ABSTRACTLAYERASSOCIATEDMODEL_H
