#include "MeshIODelegates.h"
#include "vtkPolyDataReader.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkPolyData.h"

AbstractMeshIODelegate::AbstractMeshIODelegate()
{

}


VTKMeshIODelegate::VTKMeshIODelegate()
{

}

void
VTKMeshIODelegate::LoadPolyData(const char *filename, vtkPolyData *polyData)
{
  vtkNew<vtkPolyDataReader> reader;
  reader->SetFileName(filename);
  reader->Update();
  // Using SetOutput to avoid memory issue specific to vtkPolyDataReader
  reader->SetOutput(polyData);
}

VTPMeshIODelegate::VTPMeshIODelegate()
{

}

void
VTPMeshIODelegate::LoadPolyData(const char *filename, vtkPolyData *polyData)
{
  vtkNew<vtkXMLPolyDataReader> reader;
  reader->SetFileName(filename);
  reader->Update();
  polyData = reader->GetOutput();
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
