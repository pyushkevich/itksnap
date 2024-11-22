#include "MeshIODelegates.h"
#include "vtkPolyDataReader.h"
#include "vtkXMLPolyDataReader.h"

AbstractMeshIODelegate::AbstractMeshIODelegate()
{

}


VTKMeshIODelegate::VTKMeshIODelegate()
{

}

vtkSmartPointer<vtkPolyData>
VTKMeshIODelegate::ReadPolyData(const char *filename)
{
  vtkSmartPointer<vtkPolyData> polyData;
  vtkNew<vtkPolyDataReader> reader;
  reader->SetFileName(filename);
  reader->Update();
  polyData = reader->GetOutput();
  return polyData;
}

bool
VTKMeshIODelegate::IsFilePolyData(const char *filename)
{
  // VTK file format support 5 different types of data
  // So we need to use the reader to check whether the underlying data is polydata or not
  vtkNew<vtkPolyDataReader> reader;
  reader->SetFileName(filename);
  return reader->IsFilePolyData();
}


VTPMeshIODelegate::VTPMeshIODelegate()
{

}

bool
VTPMeshIODelegate::IsFilePolyData(const char *)
{
  return true; // VTP file format is always polydata
}

vtkSmartPointer<vtkPolyData>
VTPMeshIODelegate::ReadPolyData(const char *filename)
{
  vtkSmartPointer<vtkPolyData> polyData;
  vtkNew<vtkXMLPolyDataReader> reader;
  reader->SetFileName(filename);
  reader->Update();
  polyData = reader->GetOutput();
  return polyData;
}



AbstractMeshIODelegate*
AbstractMeshIODelegate::GetDelegate(GuidedMeshIO::FileFormat fmt)
{
  switch(fmt)
    {
      case GuidedMeshIO::FORMAT_VTK:
        return new VTKMeshIODelegate();
      case GuidedMeshIO::FORMAT_VTP:
        return new VTPMeshIODelegate();
      case GuidedMeshIO::FORMAT_BYU:
      case GuidedMeshIO::FORMAT_STL:
      case GuidedMeshIO::FORMAT_VRML:
      default:
        break;
    }

  return nullptr;
}
