#include "IOActions.h"
#include "IRISApplication.h"
#include "GlobalUIModel.h"
#include "GenericImageData.h"


void LoadMainImageAction
::Initialize(GlobalUIModel *model, const char *file)
{
  UIAbstractAction::Initialize(model);
  m_File = file;
}

void LoadMainImageAction
::Execute()
{
  // Unload the old image
  SmartPtr<UnloadMainImageAction> unload = UnloadMainImageAction::New();
  unload->Initialize(m_Model);
  unload->Execute();

  // Tell the driver to load the main image
  m_Model->GetDriver()->LoadMainImage(m_File.c_str());
}


void UnloadMainImageAction::Execute()
{
  // Unload the image
  m_Model->GetDriver()->UnloadMainImage();

}

void
LoadSegmentationAction
::Initialize(GlobalUIModel *model, const char *file)
{
  UIAbstractAction::Initialize(model);
  m_File = file;
}

void
LoadSegmentationAction
::Execute()
{

}

