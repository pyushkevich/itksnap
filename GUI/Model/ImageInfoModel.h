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

  // Implementation of virtual functions from parent class
  void RegisterWithLayer(GreyImageWrapperBase *layer);
  void UnRegisterFromLayer(GreyImageWrapperBase *layer);

  // PArent model assignment override
  virtual void SetParentModel(GlobalUIModel *parent);

  // Some model types
  typedef AbstractPropertyModel<Vector3d> RealVectorSimpleModel;
  typedef AbstractPropertyModel<Vector3ui> UIntVectorSimpleModel;
  typedef AbstractRangedPropertyModel<Vector3ui>::Type UIntVectorRangedModel;
  typedef AbstractPropertyModel<Vector2d> MinMaxIntensityModel;

  typedef AbstractPropertyModel<std::string> StringValueModel;

  // Access the individual models
  irisGetMacro(ImageDimensionsModel, UIntVectorSimpleModel *)
  irisGetMacro(ImageSpacingModel, RealVectorSimpleModel *)
  irisGetMacro(ImageOriginModel, RealVectorSimpleModel *)
  irisGetMacro(ImageItkCoordinatesModel, RealVectorSimpleModel *)
  irisGetMacro(ImageNiftiCoordinatesModel, RealVectorSimpleModel *)
  irisGetMacro(ImageVoxelCoordinatesModel, UIntVectorRangedModel *)
  irisGetMacro(ImageMinMaxModel, MinMaxIntensityModel *)
  irisGetMacro(ImageOrientationModel, StringValueModel *)

protected:

  SmartPtr<RealVectorSimpleModel> m_ImageSpacingModel;
  SmartPtr<RealVectorSimpleModel> m_ImageOriginModel;
  SmartPtr<RealVectorSimpleModel> m_ImageItkCoordinatesModel;
  SmartPtr<RealVectorSimpleModel> m_ImageNiftiCoordinatesModel;
  SmartPtr<UIntVectorSimpleModel> m_ImageDimensionsModel;
  SmartPtr<UIntVectorRangedModel> m_ImageVoxelCoordinatesModel;
  SmartPtr<MinMaxIntensityModel> m_ImageMinMaxModel;
  SmartPtr<StringValueModel> m_ImageOrientationModel;

  bool GetImageDimensions(Vector3ui &value);
  bool GetImageOrigin(Vector3d &value);
  bool GetImageSpacing(Vector3d &value);
  bool GetImageItkCoordinates(Vector3d &value);
  bool GetImageNiftiCoordinates(Vector3d &value);
  bool GetImageMinMax(Vector2d &value);
  bool GetImageOrientation(std::string &value);

  bool GetImageVoxelCoordinatesValueAndRange(
      Vector3ui &value, NumericValueRange<Vector3ui> *range);

  void SetImageVoxelCoordinates(Vector3ui value);

  ImageInfoModel();
  virtual ~ImageInfoModel() {}
};

#endif // IMAGEINFOMODEL_H
