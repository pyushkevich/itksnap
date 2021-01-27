#include "ImageInfoModel.h"
#include "LayerAssociation.txx"
#include "MetaDataAccess.h"
#include <cctype>
#include <algorithm>


// This compiles the LayerAssociation for the color map
template class LayerAssociation<ImageInfoLayerProperties,
                                ImageWrapperBase,
                                ImageInfoModelBase::PropertiesFactory>;

ImageInfoModel::ImageInfoModel()
{
  m_ImageDimensionsModel = wrapGetterSetterPairAsProperty(
        this, &Self::GetImageDimensions);

  m_ImageSpacingModel = wrapGetterSetterPairAsProperty(
        this, &Self::GetImageSpacing);

  m_ImageOriginModel = wrapGetterSetterPairAsProperty(
        this, &Self::GetImageOrigin);

  m_ImageItkCoordinatesModel = wrapGetterSetterPairAsProperty(
        this, &Self::GetImageItkCoordinates);

  m_ImageNiftiCoordinatesModel = wrapGetterSetterPairAsProperty(
        this, &Self::GetImageNiftiCoordinates);

  m_ImageMinMaxModel = wrapGetterSetterPairAsProperty(
        this, &Self::GetImageMinMax);

  m_ImageOrientationModel = wrapGetterSetterPairAsProperty(
        this, &Self::GetImageOrientation);

  m_ImageNumberOfTimePointsModel = wrapGetterSetterPairAsProperty(
        this, &Self::GetImageNumberOfTimePoints);

  m_ImageCurrentTimePointModel = wrapGetterSetterPairAsProperty(
        this, &Self::GetCurrentTimePointValueAndRange, &Self::SetCurrentTimePointValue);

  m_ImageScalarIntensityUnderCursorModel = wrapGetterSetterPairAsProperty(
        this, &Self::GetImageScalarIntensityUnderCursor);

  // Create the property model for the filter
  m_MetadataFilterModel = ConcreteSimpleStringProperty::New();

  // Listen to events on the filter, so we can update the metadata
  Rebroadcast(m_MetadataFilterModel, ValueChangedEvent(), MetadataChangeEvent());

  // Also rebroadcast active layer change events as both ModelChange and Metadata
  // change events
  Rebroadcast(this, ActiveLayerChangedEvent(), MetadataChangeEvent());

  // Active layer change also impacts the states
  Rebroadcast(this, ActiveLayerChangedEvent(), StateMachineChangeEvent());
}

void ImageInfoModel::SetParentModel(GlobalUIModel *parent)
{
  Superclass::SetParentModel(parent);

  // Cursor update events are mapped to model update events
  Rebroadcast(m_ParentModel, CursorUpdateEvent(), ModelUpdateEvent());
}

bool ImageInfoModel
::GetImageDimensions(Vector3ui &value)
{
  if(!this->GetLayer()) return false;
  value = GetLayer()->GetSize();
  return true;
}

bool ImageInfoModel
::GetImageOrigin(Vector3d &value)
{
  if(!this->GetLayer()) return false;
  value = GetLayer()->GetImageBase()->GetOrigin();
  return true;
}

bool ImageInfoModel
::GetImageSpacing(Vector3d &value)
{
  if(!this->GetLayer()) return false;
  value = GetLayer()->GetImageBase()->GetSpacing();
  return true;
}

bool ImageInfoModel
::GetImageItkCoordinates(Vector3d &value)
{
  if(!this->GetLayer()) return false;
  Vector3ui cursor = m_ParentModel->GetDriver()->GetCursorPosition();
  value = GetLayer()->TransformVoxelIndexToPosition(to_int(cursor));
  return true;
}

bool ImageInfoModel
::GetImageNiftiCoordinates(Vector3d &value)
{
  if(!this->GetLayer()) return false;
  Vector3ui cursor = m_ParentModel->GetDriver()->GetCursorPosition();
  value = GetLayer()->TransformVoxelCIndexToNIFTICoordinates(to_double(cursor));
  return true;
}

bool ImageInfoModel
::GetImageMinMax(Vector2d &value)
{
  ImageWrapperBase *layer = this->GetLayer();

  // TODO: figure out how to handle this conistently throughout
  if(layer)
    {
    value = Vector2d(layer->GetImageMinNative(), layer->GetImageMaxNative());
    return true;
    }

  return false;
}

bool ImageInfoModel
::GetImageOrientation(std::string &value)
{
  if(!this->GetLayer()) return false;

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

bool ImageInfoModel::GetImageNumberOfTimePoints(unsigned int &value)
{
  ImageWrapperBase *layer = this->GetLayer();
  if(layer)
    {
    value = this->GetLayer()->GetNumberOfTimePoints();
    return true;
    }
  else return false;
}

bool ImageInfoModel::GetImageScalarIntensityUnderCursor(double &value)
{
  ImageWrapperBase *layer = this->GetLayer();
  if(layer && layer->GetNumberOfComponents() == 1 && layer->GetNumberOfTimePoints() == 1)
  {
    vnl_vector<double> ivec(1);
    layer->GetVoxelMappedToNative(layer->GetSliceIndex(), layer->GetTimePointIndex(), ivec);
    value = ivec[0];
    return true;
  }
  return false;
}

bool
ImageInfoModel
::GetCurrentTimePointValueAndRange(
    unsigned int &value,
    NumericValueRange<unsigned int> *range)
{
  ImageWrapperBase *layer = this->GetLayer();
  if(layer)
    {
    value = layer->GetTimePointIndex() + 1;
    if(range)
      range->Set(1, layer->GetNumberOfTimePoints(), 1);
    return true;
    }
  else return false;
}

void ImageInfoModel::SetCurrentTimePointValue(unsigned int value)
{
  // We can only set the timepoint when the layer has as many time points as the main
  // image since we must go through the IRISApplication to set time point index
  ImageWrapperBase *layer = this->GetLayer();
  ImageWrapperBase *main = this->GetParentModel()->GetDriver()->GetCurrentImageData()->GetMain();

  // TODO: make this an assertion
  if(main && layer && main->GetNumberOfTimePoints() == layer->GetNumberOfTimePoints())
    {
    this->GetParentModel()->GetDriver()->SetCursorTimePoint(value);
    }
}



void ImageInfoModel::OnUpdate()
{
  Superclass::OnUpdate();

  // Is there a change to the metadata?
  if(this->m_EventBucket->HasEvent(ActiveLayerChangedEvent()) ||
     this->m_EventBucket->HasEvent(ValueChangedEvent(),m_MetadataFilterModel))
    {
    // Recompute the metadata index
    this->UpdateMetadataIndex();
    }
}

bool ImageInfoModel::CheckState(ImageInfoModel::UIState state)
{
  ImageWrapperBase *layer = this->GetLayer();
  ImageWrapperBase *main = this->GetParentModel()->GetDriver()->GetMainImage();
  switch(state)
    {
    case ImageInfoModel::UIF_TIME_POINT_IS_EDITABLE:
      return layer && main
          && layer->GetNumberOfTimePoints() == main->GetNumberOfTimePoints()
          && main->GetNumberOfTimePoints() > 1;
    case ImageInfoModel::UIF_INTENSITY_IS_MULTIVALUED:
      return layer && (layer->GetNumberOfComponents() > 1 || layer->GetNumberOfTimePoints() > 1);
    }
  return false;
}

bool case_insensitive_predicate(char a, char b)
{
  return std::tolower(a) == std::tolower(b);
}

bool case_insensitive_find(std::string &a, std::string &b)
{
  std::string::iterator it = std::search(
        a.begin(), a.end(), b.begin(), b.end(), case_insensitive_predicate);
  return it != a.end();
}

void ImageInfoModel::UpdateMetadataIndex()
{
  // Clear the list of selected keys
  m_MetadataKeys.clear();

  // Search keys and values that meet the filter
  if(GetLayer())
    {
    auto mda = GetLayer()->GetMetaDataAccess();
    std::vector<std::string> keys = mda.GetKeysAsArray();
    std::string filter = m_MetadataFilterModel->GetValue();
    for(size_t i = 0; i < keys.size(); i++)
      {
      std::string key = keys[i];
      std::string dcm = mda.MapKeyToDICOM(key);
      std::string value = mda.GetValueAsString(key);

      if(filter.size() == 0 ||
         case_insensitive_find(dcm, filter) ||
         case_insensitive_find(value, filter))
        {
        m_MetadataKeys.push_back(key);
        }
      }
    }
}

int ImageInfoModel::GetMetadataRows()
{
  return m_MetadataKeys.size();
}

std::string ImageInfoModel::GetMetadataCell(int row, int col)
{
  assert(GetLayer());
  assert(row >= 0 && row < (int) m_MetadataKeys.size());
  std::string key = m_MetadataKeys[row];
  auto mda = GetLayer()->GetMetaDataAccess();
  return (col == 0) ? mda.MapKeyToDICOM(key) : mda.GetValueAsString(key);
}




