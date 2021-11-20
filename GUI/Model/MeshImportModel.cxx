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
  std::cout << "[MeshImprotModel] formatName=" << formatName << std::endl;
  for (int i = 0; i < GuidedMeshIO::FORMAT_COUNT; ++i)
    {
      FileFormat fmt = (FileFormat)i;
      if (GuidedMeshIO::m_MeshFormatDescriptorMap.at(fmt).name.compare(formatName) == 0)
        return fmt;
    }

  return GuidedMeshIO::FORMAT_COUNT;
}

void
MeshImportModel::Load(const char *filename, FileFormat format)
{
  // Create a new IO for loading
  GuidedMeshIO *IO = new GuidedMeshIO();

  // Create a mesh wrapper
  auto wrapper = StandaloneMeshWrapper::New();
  SmartPtr<MeshWrapperBase> baseWrapper = wrapper.GetPointer();

  std::cout << "[MeshImportModel.Load()] Mesh created. id=" << wrapper->GetId() << std::endl;

  // Execute loading
  IO->LoadMesh(filename, format, baseWrapper);

  // Install the wrapper to the application
  m_ParentModel->GetDriver()->GetIRISImageData()->GetMeshLayers()->AddLayer(baseWrapper);

  std::cout << "[MeshImportModel.Load()] Mesh Installed" << std::endl;

  delete IO;
}
