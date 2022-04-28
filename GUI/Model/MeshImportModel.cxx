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
MeshImportModel::Load(std::vector<std::string> &fn_list, FileFormat format)
{
  // if failed, check upstream file loading logic
  assert(fn_list.size() > 0);

  // Create a new IO for loading
  GuidedMeshIO *IO = new GuidedMeshIO();

  // Create a mesh wrapper
  auto wrapper = StandaloneMeshWrapper::New();
  SmartPtr<MeshWrapperBase> baseWrapper = wrapper.GetPointer();

  // Load into the current time point
  IRISApplication *app = m_ParentModel->GetDriver();

  if (m_Mode == Mode::SERIES)
    {
    // load each file in the list from tp 0
    size_t tp = 0, nt = app->GetNumberOfTimePoints();
    for (auto &fn : fn_list)
      {
      if (tp > nt)
        break;

      // Execute loading
      IO->LoadMesh(fn.c_str(), format, baseWrapper, tp++, 0u);
      }
    }
  else
    {
    // load file into current tp
    auto crntTP = app->GetCursorTimePoint();

    IO->LoadMesh(fn_list[0].c_str(), format, baseWrapper, crntTP, 0u);
    }

  // Install the wrapper to the application
  app->GetIRISImageData()->GetMeshLayers()->AddLayer(baseWrapper);

  delete IO;
}

void
MeshImportModel::LoadToTP(const char *filename, FileFormat format)
{
  // Create a new IO for loading
  GuidedMeshIO *IO = new GuidedMeshIO();

  // Load into the current time point
  auto app = m_ParentModel->GetDriver();
  auto tp = app->GetCursorTimePoint();
  auto layers = app->GetIRISImageData()->GetMeshLayers();
  auto active_layer = layers->GetLayer(layers->GetActiveLayerId());

  // Execute loading
  IO->LoadMesh(filename, format, active_layer, tp, 0u);

  delete IO;
}
