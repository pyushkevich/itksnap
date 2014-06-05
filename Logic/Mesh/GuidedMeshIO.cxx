#include "GuidedMeshIO.h"
#include "vtkPolyDataWriter.h"
#include "vtkSTLWriter.h"
#include "vtkBYUWriter.h"
#include "vtkTriangleFilter.h"
#include "itkExceptionObject.h"

GuidedMeshIO
::GuidedMeshIO()
{
  m_EnumFileFormat.AddPair(FORMAT_VTK, "VTK Mesh");
  m_EnumFileFormat.AddPair(FORMAT_BYU, "BYU Mesh");
  m_EnumFileFormat.AddPair(FORMAT_STL, "STL Mesh"); 
  m_EnumFileFormat.AddPair(FORMAT_VRML, "VRML Scene");
  m_EnumFileFormat.AddPair(FORMAT_COUNT, "INVALID FORMAT");
}


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
