#include "GuidedMeshIO.h"
#include "MeshIODelegates.h"
#include "MeshWrapperBase.h"
#include "StandaloneMeshWrapper.h"

#include <itkMacro.h>
#include <itksys/SystemTools.hxx>
#include <vtkDataReader.h>
#include <vtkPolyDataWriter.h>
#include <vtkSTLWriter.h>
#include <vtkBYUWriter.h>
#include <vtkTriangleFilter.h>

GuidedMeshIO
::GuidedMeshIO()
{
}

RegistryEnumMap<GuidedMeshIO::FileFormat>
GuidedMeshIO::m_EnumFileFormat =
{
  { FORMAT_VTK, "VTK Mesh" },
  { FORMAT_BYU, "BYU Mesh" },
  { FORMAT_STL, "STL Mesh" },
  { FORMAT_VRML, "VRML Scene" },
  { FORMAT_VTP, "VTP Mesh" },
  { FORMAT_COUNT, "INVALID FORMAT" }
};

const GuidedMeshIO::MeshFormatDescriptorMap
GuidedMeshIO::m_MeshFormatDescriptorMap =
{
  // FileFormat => MeshFormatDescriptor
  // Descriptor members: { name, {extensions}, can_read, can_write }
  { FORMAT_BYU, { "BYU Mesh",   {".byu", ".y"},   false,  true } },
  { FORMAT_STL, { "STL Mesh",   {".stl"},         false,  true } },
  { FORMAT_VRML,{ "VRML Scene", {".vrml"},        false,  true } },
  { FORMAT_VTK, { "VTK Mesh",   {".vtk"},         true,   true } },
  { FORMAT_VTP, { "VTP Mesh",   {".vtp"},         true,   false } }
};


GuidedMeshIO::FileFormat 
GuidedMeshIO
::GetFileFormat(Registry &folder, FileFormat dflt)
{
  return folder.Entry("Format").GetEnum(m_EnumFileFormat, dflt);  
}

GuidedMeshIO::FileFormat
GuidedMeshIO::
GetFormatByExtension(std::string extension)
{
  if (extension.empty()) // possible when reading dicom series
    return FileFormat::FORMAT_COUNT;

  // All format string in the map include '.' prefix
  if (extension.at(0) != '.')
    extension.insert(0, 1, '.');

  for (auto kv : m_MeshFormatDescriptorMap)
    {
    // Looking for the first format match the extension
    if (kv.second.extensions.count(extension))
      return kv.first;
    }

  return FileFormat::FORMAT_COUNT;
}

void GuidedMeshIO
::SetFileFormat(Registry &folder, FileFormat format)
{
  folder.Entry("Format").PutEnum(m_EnumFileFormat, format);
}

std::string
GuidedMeshIO::GetFormatDescription(FileFormat formatEnum)
{
  return m_EnumFileFormat[formatEnum];
}

bool
GuidedMeshIO::can_read(FileFormat fmt)
{
  bool ret = false;
  if (m_MeshFormatDescriptorMap.count(fmt))
    ret = m_MeshFormatDescriptorMap.at(fmt).can_read;
  return ret;
}

bool
GuidedMeshIO::can_write(FileFormat fmt)
{
  bool ret = false;
  if (m_MeshFormatDescriptorMap.count(fmt))
    ret = m_MeshFormatDescriptorMap.at(fmt).can_write;
  return ret;
}

std::string
GuidedMeshIO
::GetSlicerCoordSysComment() const
{
  // Adding a comment illustrating the purpose
  std::ostringstream oss;
  oss << "ITK-SNAP coord sys for 3D Slicer: " << slicer_coord_sys_string;
  return oss.str();
}

void
GuidedMeshIO
::SaveMesh(const char *FileName, Registry &folder, vtkPolyData *mesh)
{
  // Read the format specification from the registry folder
  FileFormat format = GetFileFormat(folder);

  // Create the appropriate mesh writer for the format
  if(format == FORMAT_VTK)
    {
    vtkPolyDataWriter *writer = vtkPolyDataWriter::New();
    writer->SetInputData(mesh);
    writer->SetFileName(FileName);
    writer->SetHeader(GetSlicerCoordSysComment().c_str());
    writer->Update();
    writer->Delete();
    }
  else if(format == FORMAT_STL)
    {
    vtkTriangleFilter *tri = vtkTriangleFilter::New();
    vtkSTLWriter *writer = vtkSTLWriter::New();
    tri->SetInputData(mesh);
    writer->SetInputConnection(tri->GetOutputPort());
    writer->SetFileName(FileName);
    writer->SetHeader(GetSlicerCoordSysComment().c_str());
    writer->Update();
    writer->Delete();
    tri->Delete();
    }
  else if(format == FORMAT_BYU)
    {
    vtkTriangleFilter *tri = vtkTriangleFilter::New();
    vtkBYUWriter *writer = vtkBYUWriter::New();
    tri->SetInputData(mesh);
    writer->SetInputConnection(tri->GetOutputPort());
    writer->SetGeometryFileName(FileName);
    writer->Update();
    writer->Delete();
    tri->Delete();
    }
  else 
    throw itk::ExceptionObject("Illegal format specified for saving image");
}

void
GuidedMeshIO::LoadMesh(const char *FileName, FileFormat format,
                       SmartPtr<MeshWrapperBase> wrapper, unsigned int tp, LabelType id)
{
  // Using the factory method to get a delegate
  AbstractMeshIODelegate *ioDelegate = AbstractMeshIODelegate::GetDelegate(format);

  if (ioDelegate)
    {
      // Apply IO logic of the delegate
      vtkSmartPointer<vtkPolyData> polyData = ioDelegate->ReadPolyData(FileName);

      // Set polydata into the wrapper
      wrapper->SetMesh(polyData, tp, id);

      // Get poly data wrapper loaded
      auto polyDataWrapper = wrapper->GetMesh(tp, id);

      polyDataWrapper->SetFileName(FileName);

      polyDataWrapper->SetFileFormat(format);

      delete ioDelegate;
    }
  else
    throw itk::ExceptionObject("Illegal format specified for loading mesh file");
}

std::string
GuidedMeshIO::GetErrorMessage() const
{
  return m_ErrorMessage;
}

bool
GuidedMeshIO
::IsFilePolyData(const char *filename)
{
  auto fmt = GetFormatByFilename(filename);
  if (fmt == FORMAT_COUNT)
    return false;

  AbstractMeshIODelegate *ioDelegate = AbstractMeshIODelegate::GetDelegate(fmt);

  if (!ioDelegate)
    return false;

  bool ret = ioDelegate->IsFilePolyData(filename);
  delete ioDelegate;
  return ret;
}

GuidedMeshIO::FileFormat
GuidedMeshIO
::GetFormatByFilename(const char *filename)
{
  // extract extension
  std::string ext = itksys::SystemTools::GetFilenameLastExtension(filename);
  return GetFormatByExtension(ext);
}
