#include "ImageInfoModel.h"
#include "LayerAssociation.txx"

// This compiles the LayerAssociation for the color map
template class LayerAssociation<ImageInfoLayerProperties,
                                GreyImageWrapperBase,
                                ImageInfoModelBase::PropertiesFactory>;

ImageInfoModel::ImageInfoModel()
{
  m_ImageDimensionsModel = makeChildNumericValueModel(
        this, &Self::GetImageDimensionsValueAndRange);

  m_ImageSpacingModel = makeChildNumericValueModel(
        this, &Self::GetImageSpacingValueAndRange);

  m_ImageOriginModel = makeChildNumericValueModel(
        this, &Self::GetImageOriginValueAndRange);

  m_ImageItkCoordinatesModel = makeChildNumericValueModel(
        this, &Self::GetImageItkCoordinatesValueAndRange);

  m_ImageNiftiCoordinatesModel = makeChildNumericValueModel(
        this, &Self::GetImageNiftiCoordinatesValueAndRange);

  m_ImageVoxelCoordinatesModel = makeChildNumericValueModel(
        this,
        &Self::GetImageVoxelCoordinatesValueAndRange,
        &Self::SetImageVoxelCoordinates);
}

void ImageInfoModel::SetParentModel(GlobalUIModel *parent)
{
  Superclass::SetParentModel(parent);

  // Cursor update events are mapped to model update events
  Rebroadcast(m_ParentModel, CursorUpdateEvent(), ModelUpdateEvent());
}


void ImageInfoModel::RegisterWithLayer(GreyImageWrapperBase *layer)
{
  // We don't need to listen to the events on the layer because they
  // are not going to change anything managed by this model.
}

void ImageInfoModel::UnRegisterFromLayer(GreyImageWrapperBase *layer)
{
  // We don't need to listen to the events on the layer because they
  // are not going to change anything managed by this model.
}

bool ImageInfoModel
::GetImageDimensionsValueAndRange(
    Vector3ui &value, NumericValueRange<Vector3ui> *range)
{
  value = GetLayer()->GetSize();
  return true;
}

bool ImageInfoModel
::GetImageOriginValueAndRange(
    Vector3d &value, NumericValueRange<Vector3d> *range)
{
  value = GetLayer()->GetImageBase()->GetOrigin();
  return true;
}

bool ImageInfoModel
::GetImageSpacingValueAndRange(
    Vector3d &value, NumericValueRange<Vector3d> *range)
{
  value = GetLayer()->GetImageBase()->GetSpacing();
  return true;
}

bool ImageInfoModel
::GetImageItkCoordinatesValueAndRange(
    Vector3d &value, NumericValueRange<Vector3d> *range)
{
  Vector3ui cursor = m_ParentModel->GetDriver()->GetCursorPosition();
  value = GetLayer()->TransformVoxelIndexToPosition(cursor);
  return true;
}

bool ImageInfoModel
::GetImageNiftiCoordinatesValueAndRange(
    Vector3d &value, NumericValueRange<Vector3d> *range)
{
  Vector3ui cursor = m_ParentModel->GetDriver()->GetCursorPosition();
  value = GetLayer()->TransformVoxelIndexToNIFTICoordinates(to_double(cursor));
  return true;
}

bool ImageInfoModel
::GetImageVoxelCoordinatesValueAndRange(
    Vector3ui &value, NumericValueRange<Vector3ui> *range)
{
  value = m_ParentModel->GetDriver()->GetCursorPosition();
  if(range)
    {
    range->Set(Vector3ui(0), GetLayer()->GetSize() - (unsigned int) 1, Vector3ui(1));
    }
  return true;
}

void ImageInfoModel::SetImageVoxelCoordinates(Vector3ui value)
{
  m_ParentModel->GetDriver()->SetCursorPosition(value);
}


