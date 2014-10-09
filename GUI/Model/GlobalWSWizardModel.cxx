#include "GlobalWSWizardModel.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"



GlobalWSWizardModel::GlobalWSWizardModel()
{
}

void GlobalWSWizardModel::SetParentModel(GlobalUIModel *model)
{
  m_Parent = model;
  m_Driver = m_Parent->GetDriver();
  m_GlobalState = m_Driver->GetGlobalState();

  // Layer changes are rebroadcast as model changes, causing all child
  // models to update themselves.
  Rebroadcast(m_Driver, LayerChangeEvent(), ModelUpdateEvent());
 }

void GlobalWSWizardModel::OnGlobalWSModeEnter()
{
  //// Initialize the image data
  m_Driver->InitializeJOINImageData(
        m_Driver->GetGlobalState()->GetSegmentationROISettings(),
        m_Parent->GetProgressCommand());

  m_Driver->SetCurrentImageDataToJOIN();
  //m_GlobalState->SetToolbarMode(CROSSHAIRS_MODE); //disables JoinDataPanel AND JoinInteraction
  m_GlobalState->SetToolbarMode(GWSJOIN_MODE); //only disables JoinDataPanel

}

void GlobalWSWizardModel::OnCancelSegmentation()
{
  // Return to IRIS mode
  m_Driver->SetCurrentImageDataToIRIS();
  m_Driver->ReleaseJOINImageData();

  m_GlobalState->SetToolbarMode(CROSSHAIRS_MODE); //disables JoinInteraction
}

void GlobalWSWizardModel::OnFinishGWS()
{
  // Update IRIS with JOIN images
  m_Driver->UpdateIRISWithJOINImageData(NULL);

  // Set an undo point
  m_Driver->StoreUndoPoint("Automatic Segmentation");

  // Return to IRIS mode
  m_Driver->SetCurrentImageDataToIRIS();
  m_Driver->ReleaseJOINImageData();

  m_GlobalState->SetToolbarMode(CROSSHAIRS_MODE); //disables JoinInteraction
}


