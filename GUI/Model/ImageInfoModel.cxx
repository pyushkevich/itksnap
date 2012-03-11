#include "ImageInfoModel.h"
#include "LayerAssociation.txx"

// This compiles the LayerAssociation for the color map
template class LayerAssociation<ImageInfoLayerProperties,
                                GreyImageWrapperBase,
                                ImageInfoModelBase::PropertiesFactory>;

ImageInfoModel::ImageInfoModel()
{
  m_ImageDimensionsModel = makeChildPropertyModel(
        this, &Self::GetImageDimensions);

  m_ImageSpacingModel = makeChildPropertyModel(
        this, &Self::GetImageSpacing);

  m_ImageOriginModel = makeChildPropertyModel(
        this, &Self::GetImageOrigin);

  m_ImageItkCoordinatesModel = makeChildPropertyModel(
        this, &Self::GetImageItkCoordinates);

  m_ImageNiftiCoordinatesModel = makeChildPropertyModel(
        this, &Self::GetImageNiftiCoordinates);

  m_ImageVoxelCoordinatesModel = makeChildPropertyModel(
        this,
        &Self::GetImageVoxelCoordinatesValueAndRange,
        &Self::SetImageVoxelCoordinates);

  m_ImageMinMaxModel = makeChildPropertyModel(
        this, &Self::GetImageMinMax);

  m_ImageOrientationModel = makeChildPropertyModel(
        this, &Self::GetImageOrientation);
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
::GetImageDimensions(Vector3ui &value)
{
  value = GetLayer()->GetSize();
  return true;
}

bool ImageInfoModel
::GetImageOrigin(Vector3d &value)
{
  value = GetLayer()->GetImageBase()->GetOrigin();
  return true;
}

bool ImageInfoModel
::GetImageSpacing(Vector3d &value)
{
  value = GetLayer()->GetImageBase()->GetSpacing();
  return true;
}

bool ImageInfoModel
::GetImageItkCoordinates(Vector3d &value)
{
  Vector3ui cursor = m_ParentModel->GetDriver()->GetCursorPosition();
  value = GetLayer()->TransformVoxelIndexToPosition(cursor);
  return true;
}

bool ImageInfoModel
::GetImageNiftiCoordinates(Vector3d &value)
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

bool ImageInfoModel
::GetImageMinMax(Vector2d &value)
{
  value = Vector2d(GetLayer()->GetImageMinNative(),
                   GetLayer()->GetImageMaxNative());
  return true;
}

bool ImageInfoModel
::GetImageOrientation(std::string &value)
{
  const ImageCoordinateGeometry &geo =
      m_ParentModel->GetDriver()->GetCurrentImageData()->GetImageGeometry();
  ImageCoordinateGeometry::DirectionMatrix dmat =
      geo.GetImageDirectionCosineMatrix();

  std::string raicode =
    ImageCoordinateGeometry::ConvertDirectionMatrixToClosestRAICode(dmat);

  if (ImageCoordinateGeometry::IsDirectionMatrixOblique(dmat))
    value = std::string("Oblique (closest to ") + raicode + string(")");
  else
    value = raicode;

  return true;
}


