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
    WrapperBase> ImageInfoModelBase;

class ImageInfoModel : public ImageInfoModelBase
{
public:
  irisITKObjectMacro(ImageInfoModel, ImageInfoModelBase)

  // An event fired when some aspect of the metadata has changed
  itkEventMacro(MetadataChangeEvent, IRISEvent)
  FIRES(MetadataChangeEvent)

  enum UIState {
    UIF_TIME_POINT_IS_EDITABLE,
    UIF_INTENSITY_IS_MULTIVALUED,
    UIF_TIME_IS_DISPLAYED
  };

  // Implementation of virtual functions from parent class
  void RegisterWithLayer(WrapperBase *layer) ITK_OVERRIDE {}
  void UnRegisterFromLayer(WrapperBase *layer, bool being_deleted) ITK_OVERRIDE {}

  // Parent model assignment override
  virtual void SetParentModel(GlobalUIModel *parent);

  // Function called in response to events
  virtual void OnUpdate() ITK_OVERRIDE;

  // Access state flags
  bool CheckState(UIState state);

  // Access the individual models
  irisGetMacro(ImageDimensionsModel, AbstractSimpleUIntVec4Property *)
  irisGetMacro(ImageSpacingModel, AbstractSimpleDoubleVec4Property *)
  irisGetMacro(ImageOriginModel, AbstractSimpleDoubleVec4Property *)
  irisGetMacro(ImageItkCoordinatesModel, AbstractSimpleDoubleVec4Property *)
  irisGetMacro(ImageNiftiCoordinatesModel, AbstractSimpleDoubleVec4Property *)
  irisGetMacro(ImageMinMaxModel, AbstractSimpleDoubleVec2Property *)
  irisGetMacro(ImageOrientationModel, AbstractSimpleStringProperty *)
  irisGetMacro(ImageNumberOfTimePointsModel, AbstractSimpleUIntProperty *)
  irisGetMacro(ImageCurrentTimePointModel, AbstractRangedUIntProperty *)
  irisGetMacro(ImagePixelFormatDescriptionModel, AbstractSimpleStringProperty *)
  irisGetMacro(ImageScalarIntensityUnderCursorModel, AbstractSimpleDoubleProperty *)

  /** This model reports whether the active layer is in reference space */
  irisGetMacro(ImageIsInReferenceSpaceModel, AbstractSimpleBooleanProperty* )

  /** This model should be used when the layer does not match the reference space */
  irisGetMacro(ImageVoxelCoordinatesObliqueModel, AbstractSimpleDoubleVec3Property *)

  // Access the internally stored filter
  irisSimplePropertyAccessMacro(MetadataFilter, std::string)

  // The voxel coordinate model just refers to the parent mode
  AbstractRangedUIntVec3Property *GetReferenceSpaceVoxelCoordinatesModel() const
  {
    return m_ParentModel->GetCursorPositionModel();
  }

  /** Number of rows in the metadata table */
  int GetMetadataRows();

  /** Get and entry in the metadata table */
  std::string GetMetadataCell(int row, int col);

protected:

  SmartPtr<AbstractSimpleBooleanProperty> m_ImageIsInReferenceSpaceModel;
  SmartPtr<AbstractSimpleDoubleVec4Property> m_ImageSpacingModel;
  SmartPtr<AbstractSimpleDoubleVec4Property> m_ImageOriginModel;
  SmartPtr<AbstractSimpleDoubleVec4Property> m_ImageItkCoordinatesModel;
  SmartPtr<AbstractSimpleDoubleVec4Property> m_ImageNiftiCoordinatesModel;
  SmartPtr<AbstractSimpleDoubleVec3Property> m_ImageVoxelCoordinatesObliqueModel;
  SmartPtr<AbstractSimpleUIntVec4Property> m_ImageDimensionsModel;
  SmartPtr<AbstractSimpleDoubleVec2Property> m_ImageMinMaxModel;
  SmartPtr<AbstractSimpleStringProperty> m_ImageOrientationModel;
  SmartPtr<ConcreteSimpleStringProperty> m_MetadataFilterModel;
  SmartPtr<AbstractSimpleUIntProperty> m_ImageNumberOfTimePointsModel;
  SmartPtr<AbstractRangedUIntProperty> m_ImageCurrentTimePointModel;
  SmartPtr<AbstractSimpleDoubleProperty> m_ImageScalarIntensityUnderCursorModel;
  SmartPtr<AbstractSimpleStringProperty> m_ImagePixelFormatDescriptionModel;


  bool GetImageIsInReferenceSpace(bool &value);
  bool GetImageDimensions(Vector4ui &value);
  bool GetImageOrigin(Vector4d &value);
  bool GetImageSpacing(Vector4d &value);
  bool GetImageItkCoordinates(Vector4d &value);
  bool GetImageNiftiCoordinates(Vector4d &value);
  bool GetImageVoxelCoordinatesOblique(Vector3d &value);
  bool GetImageMinMax(Vector2d &value);
  bool GetImageOrientation(std::string &value);
  bool GetImageNumberOfTimePoints(unsigned int &value);
  bool GetImageScalarIntensityUnderCursor(double &value);
  bool GetImagePixelFormatDescription(std::string &value);

  // Current time point model
  bool GetCurrentTimePointValueAndRange(unsigned int &value, NumericValueRange<unsigned int> *range);
  void SetCurrentTimePointValue(unsigned int value);

  // Update the list of keys managed by the metadata
  void UpdateMetadataIndex();

  // A list of metadata keys obeying the current filter
  std::vector<std::string> m_MetadataKeys;

  ImageInfoModel();
  virtual ~ImageInfoModel() {}
};

#endif // IMAGEINFOMODEL_H
