
#include "GlobalPreferencesModel.h"
#include "MeshOptions.h"
#include "GlobalUIModel.h"
#include "GlobalState.h"
#include "DefaultBehaviorSettings.h"

GlobalPreferencesModel::GlobalPreferencesModel()
{
  m_MeshOptions = MeshOptions::New();
  m_DefaultBehaviorSettings = DefaultBehaviorSettings::New();
}


void GlobalPreferencesModel::SetParentModel(GlobalUIModel *parent)
{
  m_ParentModel = parent;
}

void GlobalPreferencesModel::InitializePreferences()
{
  // Pull the preferences from the system
  GlobalState *gs = m_ParentModel->GetGlobalState();

  // Default behaviors
  m_DefaultBehaviorSettings->DeepCopy(gs->GetDefaultBehaviorSettings());

  // Mesh options
  m_MeshOptions->DeepCopy(gs->GetMeshOptions());
}

void GlobalPreferencesModel::ApplyPreferences()
{
  // Pull the preferences from the system
  GlobalState *gs = m_ParentModel->GetGlobalState();

  // Default behaviors
  gs->GetDefaultBehaviorSettings()->DeepCopy(m_DefaultBehaviorSettings);

  // Mesh options
  gs->GetMeshOptions()->DeepCopy(m_MeshOptions);
}
