#include "GuidedMeshIO.h"
#include "vtkPolyDataWriter.h"
#include "vtkSTLWriter.h"
#include "vtkBYUWriter.h"
#include "vtkTriangleFilter.h"
#include "itkMacro.h"
#include "MeshIODelegates.h"
#include "MeshWrapperBase.h"
#include "StandaloneMeshWrapper.h"

GuidedMeshIO
::GuidedMeshIO()
{
  m_EnumFileFormat.AddPair(FORMAT_VTK, "VTK Mesh");
  m_EnumFileFormat.AddPair(FORMAT_BYU, "BYU Mesh");
  m_EnumFileFormat.AddPair(FORMAT_STL, "STL Mesh"); 
  m_EnumFileFormat.AddPair(FORMAT_VRML, "VRML Scene");
  m_EnumFileFormat.AddPair(FORMAT_VTP, "VTP Mesh");
  m_EnumFileFormat.AddPair(FORMAT_COUNT, "INVALID FORMAT");
}

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
  return m_MeshFormatDescriptorMap.at(fmt).can_read;
}

bool
GuidedMeshIO::can_write(FileFormat fmt)
{
  return m_MeshFormatDescriptorMap.at(fmt).can_write;
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
GuidedMeshIO::LoadMesh(const char *FileName, FileFormat format, SmartPtr<MeshWrapperBase> wrapper)
{
  std::cout << "[GuidedMeshIO.LoadMesh] filename=" << FileName
            << "; Format=" << format << std::endl;

  // Using the factory method to get a delegate
  AbstractMeshIODelegate *ioDelegate = AbstractMeshIODelegate::GetDelegate(format);

  std::cout << "[GuidedMeshIO.LoadMesh] IODelegate created" << std::endl;

  if (ioDelegate)
    {
      // Apply IO logic of the delegate
      vtkSmartPointer<vtkPolyData> polyData = ioDelegate->ReadPolyData(FileName);
      //ioDelegate->LoadPolyData(FileName, polyData);

      std::cout << "[GuidedMeshIO.LoadMesh] vtkPolyData print" << std::endl;
      polyData->PrintSelf(std::cout, vtkIndent(2));

      std::cout << "[GuidedMeshIO.LoadMesh] PolyData loaded" << std::endl;

      wrapper->SetMesh(polyData, 1u, 1u);

      delete ioDelegate;

      std::cout << "[GuidedMeshIO.LoadMesh] Mesh installed" << std::endl;
    }
  else
    throw itk::ExceptionObject("Illegal format specified for loading mesh file");
}

std::string
GuidedMeshIO::GetErrorMessage() const
{
  return m_ErrorMessage;
}
