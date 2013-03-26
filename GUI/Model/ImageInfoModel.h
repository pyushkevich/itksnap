#ifndef IMAGEINFOMODEL_H
#define IMAGEINFOMODEL_H

#include "AbstractLayerAssociatedModel.h"
#include "PropertyModel.h"

class ImageInfoLayerProperties
{
public:
  irisGetSetMacro(ObserverTag, unsigned long)

  virtual ~ImageInfoLayerProperties() {}

protected:

  // Whether or not we are already listening to events from this layer
  unsigned long m_ObserverTag;
};

typedef AbstractLayerAssociatedModel<
    ImageInfoLayerProperties,
    ImageWrapperBase> ImageInfoModelBase;

class ImageInfoModel : public ImageInfoModelBase
{
public:
  irisITKObjectMacro(ImageInfoModel, ImageInfoModelBase)

  // An event fired when some aspect of the metadata has changed
  itkEventMacro(MetadataChangeEvent, IRISEvent)
  FIRES(MetadataChangeEvent)

  // Implementation of virtual functions from parent class
  void RegisterWithLayer(ImageWrapperBase *layer) {}
  void UnRegisterFromLayer(ImageWrapperBase *layer, bool being_deleted) {}

  // Parent model assignment override
  virtual void SetParentModel(GlobalUIModel *parent);

  // Function called in response to events
  virtual void OnUpdate();

  // Access the individual models
  irisGetMacro(ImageDimensionsModel, AbstractSimpleUIntVec3Property *)
  irisGetMacro(ImageSpacingModel, AbstractSimpleDoubleVec3Property *)
  irisGetMacro(ImageOriginModel, AbstractSimpleDoubleVec3Property *)
  irisGetMacro(ImageItkCoordinatesModel, AbstractSimpleDoubleVec3Property *)
  irisGetMacro(ImageNiftiCoordinatesModel, AbstractSimpleDoubleVec3Property *)
  irisGetMacro(ImageMinMaxModel, AbstractSimpleDoubleVec2Property *)
  irisGetMacro(ImageOrientationModel, AbstractSimpleStringProperty *)

  // Access the internally stored filter
  irisSimplePropertyAccessMacro(MetadataFilter, std::string)

  // The voxel coordinate model just refers to the parent mode
  AbstractRangedUIntVec3Property *GetImageVoxelCoordinatesModel() const
  {
    return m_ParentModel->GetCursorPositionModel();
  }


  /** Number of rows in the metadata table */
  int GetMetadataRows();

  /** Get and entry in the metadata table */
  std::string GetMetadataCell(int row, int col);

protected:

  SmartPtr<AbstractSimpleDoubleVec3Property> m_ImageSpacingModel;
  SmartPtr<AbstractSimpleDoubleVec3Property> m_ImageOriginModel;
  SmartPtr<AbstractSimpleDoubleVec3Property> m_ImageItkCoordinatesModel;
  SmartPtr<AbstractSimpleDoubleVec3Property> m_ImageNiftiCoordinatesModel;
  SmartPtr<AbstractSimpleUIntVec3Property> m_ImageDimensionsModel;
  SmartPtr<AbstractSimpleDoubleVec2Property> m_ImageMinMaxModel;
  SmartPtr<AbstractSimpleStringProperty> m_ImageOrientationModel;
  SmartPtr<ConcreteSimpleStringProperty> m_MetadataFilterModel;

  bool GetImageDimensions(Vector3ui &value);
  bool GetImageOrigin(Vector3d &value);
  bool GetImageSpacing(Vector3d &value);
  bool GetImageItkCoordinates(Vector3d &value);
  bool GetImageNiftiCoordinates(Vector3d &value);
  bool GetImageMinMax(Vector2d &value);
  bool GetImageOrientation(std::string &value);

  // Update the list of keys managed by the metadata
  void UpdateMetadataIndex();

  // A list of metadata keys obeying the current filter
  std::vector<std::string> m_MetadataKeys;

  ImageInfoModel();
  virtual ~ImageInfoModel() {}
};

#endif // IMAGEINFOMODEL_H
