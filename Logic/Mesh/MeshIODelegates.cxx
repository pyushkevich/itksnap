#include "MeshIODelegates.h"
#include "GuidedMeshIO.h"
#include "IRISException.h"
#include "MeshWrapperBase.h"
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
VTKMeshIODelegate::LoadMesh(const char *filename, vtkPolyData *polyData)
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
VTPMeshIODelegate::LoadMesh(const char *filename, vtkPolyData *polyData)
{
  vtkNew<vtkXMLPolyDataReader> reader;
  reader->SetFileName(filename);
  reader->Update();
  polyData = reader->GetOutput();
}

MeshIODelegateFactory::MeshIODelegateFactory()
{

}

AbstractMeshIODelegate*
MeshIODelegateFactory::GetDelegate(GuidedMeshIO::FileFormat FileFormat)
{
  AbstractMeshIODelegate *ret = nullptr;

  switch(FileFormat)
    {
      case GuidedMeshIO::FORMAT_VTK:
        ret = VTKMeshIODelegate::New();
        break;
      case GuidedMeshIO::FORMAT_VTP:
        ret = VTPMeshIODelegate::New();
        break;
      case GuidedMeshIO::FORMAT_BYU:
      case GuidedMeshIO::FORMAT_STL:
      case GuidedMeshIO::FORMAT_VRML:
      default:
        break;
    }

  return ret;
}
