#include "MeshImportModel.h"
#include "GlobalUIModel.h"
#include "MeshWrapperBase.h"
#include "StandaloneMeshWrapper.h"
#include "IRISApplication.h"
#include "IRISImageData.h"
#include "ImageMeshLayers.h"

MeshImportModel::MeshImportModel()
{
  // Set a default mode
  m_Mode = SINGLE;

  // Configure the import model
  m_ImportFormatModel = ConcreteFileFormatModel::New();
  m_ImportFormatModel->SetValue(GuidedMeshIO::FORMAT_VTK);

  // Set File Format Domain
  // Set the list of file formats supported by the import model here
  // key is the GuidedMeshIO format, value is the description of the format
  FileFormatDomain ffd;
  for (int i = 0; i < GuidedMeshIO::FORMAT_COUNT; ++i)
    {
      FileFormat fmt = (FileFormat)i;

      // Only add readable formats (readability will not be checked later)
      if (GuidedMeshIO::can_read(fmt))
        {
          ffd[fmt] = GuidedMeshIO::m_MeshFormatDescriptorMap.at(fmt).name;
        }
    }
  m_ImportFormatModel->SetDomain(ffd);

  m_FormatRegExp[GuidedMeshIO::FORMAT_VTK] = ".*\\.vtk$";
  m_FormatRegExp[GuidedMeshIO::FORMAT_VTP] = ".*\\.vtp$";
}

MeshImportModel::~MeshImportModel()
{
}

void MeshImportModel::SetParentModel(GlobalUIModel *parent)
{
  this->m_ParentModel = parent;
}

GlobalUIModel* MeshImportModel::GetParentModel()
{
  return this->m_ParentModel;
}

std::string MeshImportModel::GetTitle() const
{
  if (m_Mode == Mode::SINGLE)
    return "Open a Mesh";
  else if (m_Mode == Mode::SERIES)
    return "Open a Mesh Series";
  else
    return "Open a File";
}

MeshImportModel::FileFormat
MeshImportModel::GetFileFormatByName(const std::string &formatName) const
{
  for (int i = 0; i < GuidedMeshIO::FORMAT_COUNT; ++i)
    {
      FileFormat fmt = (FileFormat)i;
      if (GuidedMeshIO::m_MeshFormatDescriptorMap.at(fmt).name.compare(formatName) == 0)
        return fmt;
    }

  return GuidedMeshIO::FORMAT_COUNT;
}

void
MeshImportModel::Load(std::vector<std::string> &fn_list, FileFormat format,
                      unsigned int loadFromTP)
{
  // if failed, check upstream file loading logic
  assert(fn_list.size() > 0);

  // Load into the current time point
  IRISApplication *app = m_ParentModel->GetDriver();
  assert(loadFromTP <= app->GetNumberOfTimePoints());

  // Main should be loaded
  if (!app->IsMainImageLoaded())
    return;

  // Get image mesh layers
  ImageMeshLayers *mesh_layers = app->GetIRISImageData()->GetMeshLayers();

  mesh_layers->AddLayerFromFiles(fn_list, format, loadFromTP);
}

void
MeshImportModel::LoadToTP(const char *filename, FileFormat format)
{

  // Load into the current time point
  auto app = m_ParentModel->GetDriver();
  auto layers = app->GetIRISImageData()->GetMeshLayers();
  auto tp = app->GetCursorTimePoint();
  auto active_layer_id = layers->GetActiveLayerId();
  auto layer = layers->GetLayer(active_layer_id);

  // This should never happen
  // If failed, check GetActiveLayerId why it's returning a non-exist layer_id
  assert(layer);

  // Get the mesh_count as the id for the new mesh (id is zero based)
  auto mesh_count = layer->GetNumberOfMeshes(tp);

  // Execute loading
  layers->LoadFileToLayer(filename, format, active_layer_id, tp, mesh_count);
}

bool
MeshImportModel
::IsActiveMeshStandalone()
{
  auto meshLayers = m_ParentModel->GetDriver()->GetIRISImageData()->GetMeshLayers();
  auto activeMesh = meshLayers->GetLayer(meshLayers->GetActiveLayerId());
  auto standaloneMesh = dynamic_cast<StandaloneMeshWrapper*>(activeMesh.GetPointer());
  return standaloneMesh != nullptr;
}
