#include "ReorientImageModel.h"
#include "GlobalUIModel.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "GenericSliceModel.h"




ReorientImageModel::ReorientImageModel()
{
  // Create the sub-models
  m_NewRAICodeModel = ConcreteSimpleStringProperty::New();
  m_NewRAICodeModel->SetValue("");

  m_CurrentRAICodeModel = makeChildPropertyModel(
        this,
        &Self::GetCurrentRAICodeValue);

}

/** Initialize with the parent model */
void ReorientImageModel::SetParentModel(GlobalUIModel *parent)
{
  // Store the model
  m_Parent = parent;

  // Listen to changes to the main image
  Rebroadcast(m_Parent->GetDriver(),
              MainImageDimensionsChangeEvent(), ModelUpdateEvent());

  // Listen to changes to the main image
  Rebroadcast(m_Parent->GetDriver(),
              MainImagePoseChangeEvent(), ModelUpdateEvent());
}

void ReorientImageModel::ApplyCurrentRAI()
{
  IRISApplication *driver = m_Parent->GetDriver();

  // Check that the current RAI is valid
  std::string rai = this->m_NewRAICodeModel->GetValue();
  assert(ImageCoordinateGeometry::IsRAICodeValid(rai.c_str()));

  // Convert the rai code to a direction matrix
  ImageCoordinateGeometry::DirectionMatrix dm =
      ImageCoordinateGeometry::ConvertRAICodeToDirectionMatrix(rai);

  // Set the direction in the image
  driver->ReorientImage(dm);

  // Tell the display slices to reinitialize
  for(int i = 0; i < 3; i++)
    {
    m_Parent->GetSliceModel(i)->InitializeSlice(driver->GetCurrentImageData());
    }
}

bool ReorientImageModel::GetCurrentRAICodeValue(std::string &value)
{
  IRISApplication *app = m_Parent->GetDriver();
  if(app->GetCurrentImageData()->IsMainLoaded())
    {
    value = app->GetImageToAnatomyRAI();
    return true;
    }
  return false;
}

bool
ReorientImageModel
::GetDirectionMatrix(
    const std::string &rai, Matrix3d &outMatrix) const
{
  // Check whether the RAI is valid
  if(!ImageCoordinateGeometry::IsRAICodeValid(rai.c_str()))
    return false;

}
