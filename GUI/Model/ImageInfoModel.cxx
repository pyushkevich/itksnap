#include "ImageInfoModel.h"
#include "LayerAssociation.txx"
#include "MetaDataAccess.h"
#include <cctype>
#include <algorithm>


// This compiles the LayerAssociation for the color map
template class LayerAssociation<ImageInfoLayerProperties,
                                WrapperBase,
                                ImageInfoModelBase::PropertiesFactory>;

ImageInfoModel::ImageInfoModel()
{
  m_ImageIsInReferenceSpaceModel = wrapGetterSetterPairAsProperty(
        this, &Self::GetImageIsInReferenceSpace);

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

  m_ImageVoxelCoordinatesObliqueModel = wrapGetterSetterPairAsProperty(
        this, &Self::GetImageVoxelCoordinatesOblique);

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
  Rebroadcast(this, ActiveLayerChangedEvent(), ModelUpdateEvent());
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
::GetImageDimensions(Vector4ui &value)
{
  ImageWrapperBase *l = dynamic_cast<ImageWrapperBase*>(this->GetLayer());

  if(!l) return false;

  // Set the spatial dimensions
  for(unsigned int k = 0; k < 3; k++)
    value[k] = l->GetSize()[k];

  // Set the time dimension
  value[3] = l->GetNumberOfTimePoints();

  return true;
}

bool ImageInfoModel
::GetImageOrigin(Vector4d &value)
{
  ImageWrapperBase *l = dynamic_cast<ImageWrapperBase*>(this->GetLayer());

  if(!l) return false;

  // Set the spatial and temporal origin at once
  for(unsigned int k = 0; k < 4; k++)
    value[k] = l->GetImage4DBase()->GetOrigin()[k];

  return true;
}

bool ImageInfoModel
::GetImageSpacing(Vector4d &value)
{
  ImageWrapperBase *l = dynamic_cast<ImageWrapperBase*>(this->GetLayer());

  if(!l) return false;

  if(!this->GetLayer()) return false;

  // Set the spatial and temporal origin at once
  for(unsigned int k = 0; k < 4; k++)
    value[k] = l->GetImage4DBase()->GetSpacing()[k];
  return true;
}

bool ImageInfoModel
::GetImageItkCoordinates(Vector4d &value)
{
  ImageWrapperBase *l = dynamic_cast<ImageWrapperBase*>(this->GetLayer());

  if(!l) return false;

  Vector3ui cursor = m_ParentModel->GetDriver()->GetCursorPosition();
  Vector3d x = l->TransformVoxelIndexToPosition(to_int(cursor));

  for(unsigned int i = 0; i < 3; i++)
    value[i] = x[i];

  value[3] = l->GetTimePointIndex() * l->GetImage4DBase()->GetSpacing()[3] +  l->GetImage4DBase()->GetOrigin()[3];
  return true;
}

bool ImageInfoModel
::GetImageNiftiCoordinates(Vector4d &value)
{
  ImageWrapperBase *l = dynamic_cast<ImageWrapperBase*>(this->GetLayer());

  if(!l) return false;

  Vector3ui cursor = m_ParentModel->GetDriver()->GetCursorPosition();
  Vector3d x = l->TransformVoxelCIndexToNIFTICoordinates(to_double(cursor));

  for(unsigned int i = 0; i < 3; i++)
    value[i] = x[i];

  value[3] = l->GetTimePointIndex() * l->GetImage4DBase()->GetSpacing()[3] +  l->GetImage4DBase()->GetOrigin()[3];

  return true;
}

bool ImageInfoModel::GetImageVoxelCoordinatesOblique(Vector3d &value)
{
  ImageWrapperBase *l = dynamic_cast<ImageWrapperBase*>(this->GetLayer());

  if(!l) return false;

  // Get the cursor coordinate in reference space units
  itk::Index<3> x_ref = l->GetSliceIndex();
  itk::ContinuousIndex<double, 3> x_img;
  l->TransformReferenceIndexToWrappedImageContinuousIndex(x_ref, x_img);

  // Set everything
  for(unsigned int d = 0; d < 3; d++)
    value[d] = x_img[d];

  return true;
}

bool ImageInfoModel
::GetImageMinMax(Vector2d &value)
{
  ImageWrapperBase *l = dynamic_cast<ImageWrapperBase*>(this->GetLayer());

  if(!l) return false;

  // TODO: figure out how to handle this conistently throughout
  value = Vector2d(l->GetImageMinNative(), l->GetImageMaxNative());
  return true;
}

bool ImageInfoModel
::GetImageOrientation(std::string &value)
{
  if(!this->GetLayer()) return false;

  if(!m_ParentModel->GetDriver()->GetCurrentImageData()->IsMainLoaded())
    return false;

  const ImageCoordinateGeometry *geo =
      m_ParentModel->GetDriver()->GetCurrentImageData()->GetImageGeometry();
  ImageCoordinateGeometry::DirectionMatrix dmat =
      geo->GetImageDirectionCosineMatrix();

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
  ImageWrapperBase *l = dynamic_cast<ImageWrapperBase*>(this->GetLayer());

  if(!l) return false;

  value = l->GetNumberOfTimePoints();
  return true;

}

bool ImageInfoModel::GetImageScalarIntensityUnderCursor(double &value)
{
  ImageWrapperBase *l = dynamic_cast<ImageWrapperBase*>(this->GetLayer());

  if(!l) return false;

  if(l && l->GetNumberOfComponents() == 1 && l->GetNumberOfTimePoints() == 1)
  {
    vnl_vector<double> ivec(1);
    l->SampleIntensityAtReferenceIndex(
          l->GetSliceIndex(), l->GetTimePointIndex(), true, ivec);
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
  ImageWrapperBase *l = dynamic_cast<ImageWrapperBase*>(this->GetLayer());

  if(!l) return false;


  value = l->GetTimePointIndex() + 1;
  if(range)
    range->Set(1, l->GetNumberOfTimePoints(), 1);
  return true;
}

void ImageInfoModel::SetCurrentTimePointValue(unsigned int value)
{
  // We can only set the timepoint when the layer has as many time points as the main
  // image since we must go through the IRISApplication to set time point index
  ImageWrapperBase *layer = dynamic_cast<ImageWrapperBase*>(this->GetLayer());
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
  ImageWrapperBase *layer = dynamic_cast<ImageWrapperBase*>(this->GetLayer());
  ImageWrapperBase *main = this->GetParentModel()->GetDriver()->GetMainImage();
  switch(state)
    {
    case ImageInfoModel::UIF_TIME_POINT_IS_EDITABLE:
      return layer && main
          && layer->GetNumberOfTimePoints() == main->GetNumberOfTimePoints()
          && main->GetNumberOfTimePoints() > 1;
    case ImageInfoModel::UIF_INTENSITY_IS_MULTIVALUED:
      return layer && (layer->GetNumberOfComponents() > 1 || layer->GetNumberOfTimePoints() > 1);
    case ImageInfoModel::UIF_TIME_IS_DISPLAYED:
      return main && layer
          && (layer->GetNumberOfTimePoints() > 1 || main->GetNumberOfTimePoints() > 1);
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

  ImageWrapperBase *layer = dynamic_cast<ImageWrapperBase*>(this->GetLayer());

  // Search keys and values that meet the filter
  if(layer)
    {
    auto mda = layer->GetMetaDataAccess();
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
  else
    {
    MeshWrapperBase *mesh_layer = dynamic_cast<MeshWrapperBase*>(this->GetLayer());
    if (!mesh_layer)
      return;

    mesh_layer->UpdateMetaData();
    auto mesh_meta = mesh_layer->GetMeshMetaData();

    for (auto kv : mesh_meta)
      m_MetadataKeys.push_back(kv.first);
    }
}

int ImageInfoModel::GetMetadataRows()
{
  return m_MetadataKeys.size();
}

std::string ImageInfoModel::GetMetadataCell(int row, int col)
{
  ImageWrapperBase *layer = dynamic_cast<ImageWrapperBase*>(this->GetLayer());
  MeshWrapperBase *mesh_layer = dynamic_cast<MeshWrapperBase*>(this->GetLayer());
  assert(layer || mesh_layer);
  assert(row >= 0 && row < (int) m_MetadataKeys.size());

  std::string value;
  std::string key = m_MetadataKeys[row];

  if (layer)
    {
    auto mda = layer->GetMetaDataAccess();
    value = (col == 0) ? mda.MapKeyToDICOM(key) : mda.GetValueAsString(key);
    }
  else
    {
    auto mesh_meta = mesh_layer->GetMeshMetaData();

    if (col == 0)
      value = key;
    else
      value = (mesh_meta.count(key)) ? mesh_meta[key] : "";
    }

  return value;
}

bool ImageInfoModel::GetImageIsInReferenceSpace(bool &value)
{
  ImageWrapperBase *layer = dynamic_cast<ImageWrapperBase*>(this->GetLayer());
  if(layer)
    {
    value = layer->ImageSpaceMatchesReferenceSpace();
    return true;
    }
  return false;
}




