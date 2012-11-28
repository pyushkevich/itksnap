#include "ReorientImageModel.h"
#include "GlobalUIModel.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "GenericImageData.h"

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
