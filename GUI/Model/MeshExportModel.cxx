#include "MeshExportModel.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "MeshExportSettings.h"
#include "Registry.h"
#include "GuidedMeshIO.h"
#include "Generic3DModel.h"
#include "itksys/RegularExpression.hxx"
#include "HistoryManager.h"

MeshExportModel::MeshExportModel()
  : AbstractModel()
{
  m_SaveMode = SAVE_SINGLE_LABEL;
  m_SaveModeModel = wrapGetterSetterPairAsProperty(
        this, &Self::GetSaveModeValue, &Self::SetSaveModeValue);

  m_ExportFileNameModel = wrapGetterSetterPairAsProperty(
        this, &Self::GetExportFileNameValue, &Self::SetExportFileNameValue);

  m_ExportedLabelModel = ConcreteColorLabelPropertyModel::New();

  // Configure the export model
  m_ExportFormatModel = ConcreteFileFormatModel::New();
  m_ExportFormatModel->SetValue(GuidedMeshIO::FORMAT_VTK);

  // Configure the format regular expressions
  m_FormatRegExp[GuidedMeshIO::FORMAT_VTK] = ".*\\.vtk$";
  m_FormatRegExp[GuidedMeshIO::FORMAT_STL] = ".*\\.stl$";
  m_FormatRegExp[GuidedMeshIO::FORMAT_BYU] = ".*\\.(byu|y)$";
  m_FormatRegExp[GuidedMeshIO::FORMAT_VRML] = ".*\\.vrml$";
}


void MeshExportModel::SetParentModel(GlobalUIModel *parent)
{
  // Store the parent model
  m_ParentModel = parent;

  // Initialize the label property
  m_ExportedLabelModel->Initialize(parent->GetDriver()->GetColorLabelTable());



}

bool MeshExportModel::CheckState(MeshExportModel::UIState flag)
{
  switch(flag)
    {
    case MeshExportModel::UIF_LABEL_SELECTION_ACTIVE:
      return this->GetSaveMode() == SAVE_SINGLE_LABEL;
    }
  return false;
}



void MeshExportModel::OnDialogOpen()
{
  // Use the drawing label as the default
  LabelType label = m_ParentModel->GetGlobalState()->GetDrawingColorLabel();
  if(label == 0)
    label = m_ParentModel->GetDriver()->GetColorLabelTable()->FindNextValidLabel(0, false);
  m_ExportedLabelModel->SetValue(label);

  // Get the default filename for the currently loaded image
  m_ExportFileNameModel->SetValue("");

  // Update the file formats list
  UpdateFormatDomain();
}

void MeshExportModel::SaveMesh()
{
  IRISApplication *app = m_ParentModel->GetDriver();
  MeshExportSettings settings;
  settings.SetMeshFileName(m_ExportFileName);
  switch(this->GetSaveMode())
    {
    case MeshExportModel::SAVE_SINGLE_LABEL:
      settings.SetFlagSingleLabel(true);
      settings.SetFlagSingleScene(false);
      settings.SetExportLabel(this->GetExportedLabel());
      break;
    case MeshExportModel::SAVE_MULTIPLE_FILES:
      settings.SetFlagSingleLabel(false);
      settings.SetFlagSingleScene(false);
      break;
    case MeshExportModel::SAVE_SCENE:
      settings.SetFlagSingleLabel(false);
      settings.SetFlagSingleScene(true);
      break;
    }

  // Handle the format (in a round-about way)
  Registry registry;
  GuidedMeshIO io;
  io.SetFileFormat(registry, this->GetExportFormat());
  settings.SetMeshFormat(registry);

  // The actual export should use the Generic3DModel, which will take care of
  // thread safety (make sure the meshes are not being updated during export
  m_ParentModel->GetModel3D()->ExportMesh(settings);

  // Update the history
  m_ParentModel->GetSystemInterface()->GetHistoryManager()->UpdateHistory(
        this->GetHistoryName(), m_ExportFileName, true);
}

MeshExportModel::FileFormat MeshExportModel::GetFileFormatByName(const std::string &name) const
{
  FileFormatDomain dom;
  FileFormat dummy;
  m_ExportFormatModel->GetValueAndDomain(dummy, &dom);

  for(int i = 0; i < GuidedMeshIO::FORMAT_COUNT; i++)
    {
    FileFormat fmt = (FileFormat) i;
    if(dom[fmt] == name)
      return fmt;
    }

  return GuidedMeshIO::FORMAT_COUNT;
}

void MeshExportModel::UpdateFormatDomain()
{
  FileFormatDomain format_domain;
  if(GetSaveMode() == SAVE_SCENE)
    {
    format_domain[GuidedMeshIO::FORMAT_VTK] = "VTK PolyData File";
    format_domain[GuidedMeshIO::FORMAT_VRML] = "VRML 2.0 File";
    }
  else
    {
    format_domain[GuidedMeshIO::FORMAT_VTK] = "VTK PolyData File";
    format_domain[GuidedMeshIO::FORMAT_STL] = "STL Mesh File";
    format_domain[GuidedMeshIO::FORMAT_BYU] = "BYU Mesh File";
    }

  m_ExportFormatModel->SetDomain(format_domain);
}

bool MeshExportModel::GetSaveModeValue(SaveMode &value)
{
  value = m_SaveMode;
  return true;
}

void MeshExportModel::SetSaveModeValue(SaveMode value)
{
  m_SaveMode = value;
  UpdateFormatDomain();
  InvokeEvent(StateMachineChangeEvent());
}

bool MeshExportModel::GetExportFileNameValue(std::string &value)
{
  value = m_ExportFileName;
  return true;
}


void MeshExportModel::SetExportFileNameValue(std::string value)
{
  m_ExportFileName = value;

  // Try to determine the file format from the extension
  for(int format = 0; format < GuidedMeshIO::FORMAT_COUNT; format++)
    {
    std::string regexp_str = m_FormatRegExp[(FileFormat) format];
    itksys::RegularExpression regexp(regexp_str.c_str());
    if(regexp.find(m_ExportFileName))
      {
      m_ExportFormatModel->SetValue((FileFormat) format);
      }
    }
}
