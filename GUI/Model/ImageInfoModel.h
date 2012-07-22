#ifndef IMAGEINFOMODEL_H
#define IMAGEINFOMODEL_H

#include "AbstractLayerAssociatedModel.h"
#include "PropertyModel.h"
#include "GreyImageWrapper.h"

class ImageInfoLayerProperties
{
public:
  irisGetSetMacro(ObserverTag, unsigned long)

protected:

  // Whether or not we are already listening to events from this layer
  unsigned long m_ObserverTag;
};

typedef AbstractLayerAssociatedModel<
    ImageInfoLayerProperties,
    GreyImageWrapperBase> ImageInfoModelBase;

class ImageInfoModel : public ImageInfoModelBase
{
public:
  irisITKObjectMacro(ImageInfoModel, ImageInfoModelBase)

  // An event fired when some aspect of the metadata has changed
  itkEventMacro(MetadataChangeEvent, IRISEvent)
  FIRES(MetadataChangeEvent)

  // Implementation of virtual functions from parent class
  void RegisterWithLayer(GreyImageWrapperBase *layer);
  void UnRegisterFromLayer(GreyImageWrapperBase *layer);

  // PArent model assignment override
  virtual void SetParentModel(GlobalUIModel *parent);

  // Function called in response to events
  virtual void OnUpdate();

  // Some model types
  typedef AbstractPropertyModel<Vector3d> RealVectorSimpleModel;
  typedef AbstractPropertyModel<Vector3ui> UIntVectorSimpleModel;
  typedef AbstractRangedPropertyModel<Vector3ui>::Type UIntVectorRangedModel;
  typedef AbstractPropertyModel<Vector2d> MinMaxIntensityModel;

  typedef AbstractPropertyModel<std::string> StringValueModel;

  // A model that contains the filter for metadata searches
  typedef ConcretePropertyModel<std::string> FilterModel;

  // Access the individual models
  irisGetMacro(ImageDimensionsModel, UIntVectorSimpleModel *)
  irisGetMacro(ImageSpacingModel, RealVectorSimpleModel *)
  irisGetMacro(ImageOriginModel, RealVectorSimpleModel *)
  irisGetMacro(ImageItkCoordinatesModel, RealVectorSimpleModel *)
  irisGetMacro(ImageNiftiCoordinatesModel, RealVectorSimpleModel *)
  irisGetMacro(ImageMinMaxModel, MinMaxIntensityModel *)
  irisGetMacro(ImageOrientationModel, StringValueModel *)
  irisGetMacro(MetadataFilterModel, FilterModel *)

  // The voxel coordinate model just refers to the parent mode
  UIntVectorRangedModel *GetImageVoxelCoordinatesModel() const
  {
    return m_ParentModel->GetCursorPositionModel();
  }


  /** Number of rows in the metadata table */
  int GetMetadataRows();

  /** Get and entry in the metadata table */
  std::string GetMetadataCell(int row, int col);

protected:

  SmartPtr<RealVectorSimpleModel> m_ImageSpacingModel;
  SmartPtr<RealVectorSimpleModel> m_ImageOriginModel;
  SmartPtr<RealVectorSimpleModel> m_ImageItkCoordinatesModel;
  SmartPtr<RealVectorSimpleModel> m_ImageNiftiCoordinatesModel;
  SmartPtr<UIntVectorSimpleModel> m_ImageDimensionsModel;
  SmartPtr<MinMaxIntensityModel> m_ImageMinMaxModel;
  SmartPtr<StringValueModel> m_ImageOrientationModel;
  SmartPtr<FilterModel> m_MetadataFilterModel;

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
