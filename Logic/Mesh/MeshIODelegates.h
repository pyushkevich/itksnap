#ifndef MESHIODELEGATES_H
#define MESHIODELEGATES_H

#include "GuidedMeshIO.h"
#include "vtkPolyData.h"

class MeshWrapperBase;

class AbstractMeshIODelegate
{
public:
  AbstractMeshIODelegate();
  virtual ~AbstractMeshIODelegate() = default;

  virtual vtkSmartPointer<vtkPolyData> ReadPolyData(const char *filename) = 0;

  static AbstractMeshIODelegate *GetDelegate(GuidedMeshIO::FileFormat fmt);

  static void GetDelegate(GuidedMeshIO::FileFormat fmt, AbstractMeshIODelegate *delegate);


};

class VTKMeshIODelegate : public AbstractMeshIODelegate
{
public:
  VTKMeshIODelegate();
  ~VTKMeshIODelegate() = default;

  vtkSmartPointer<vtkPolyData> ReadPolyData(const char *filename) override;
};

class VTPMeshIODelegate : public AbstractMeshIODelegate
{
public:
  VTPMeshIODelegate();
  virtual ~VTPMeshIODelegate() = default;

  vtkSmartPointer<vtkPolyData> ReadPolyData(const char *filename) override;
};

#endif // MESHIODELEGATES_H
