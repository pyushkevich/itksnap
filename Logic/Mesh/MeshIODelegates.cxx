#include "MeshIODelegates.h"
#include "vtkPolyDataReader.h"
#include "vtkXMLPolyDataReader.h"

AbstractMeshIODelegate::AbstractMeshIODelegate()
{

}


VTKMeshIODelegate::VTKMeshIODelegate()
{

}

void
VTKMeshIODelegate::LoadPolyData(const char *filename, vtkSmartPointer<vtkPolyData> polyData)
{
  vtkNew<vtkPolyDataReader> reader;
  reader->SetFileName(filename);
  reader->Update();
  polyData = reader->GetOutput();
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

VTPMeshIODelegate::VTPMeshIODelegate()
{

}

void
VTPMeshIODelegate::LoadPolyData(const char *filename, vtkSmartPointer<vtkPolyData> polyData)
{
  vtkNew<vtkXMLPolyDataReader> reader;
  reader->SetFileName(filename);
  reader->Update();
  polyData = reader->GetOutput();
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
