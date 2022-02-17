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

  /** Load mesh data into the out pointer */
  virtual void LoadPolyData(const char *filename, vtkSmartPointer<vtkPolyData> polyData) = 0;

  virtual vtkSmartPointer<vtkPolyData> ReadPolyData(const char *filename) = 0;

  static AbstractMeshIODelegate *GetDelegate(GuidedMeshIO::FileFormat fmt);

  static void GetDelegate(GuidedMeshIO::FileFormat fmt, AbstractMeshIODelegate *delegate);


};

class VTKMeshIODelegate : public AbstractMeshIODelegate
{
public:
  VTKMeshIODelegate();
  ~VTKMeshIODelegate() = default;

  void LoadPolyData(const char *filename, vtkSmartPointer<vtkPolyData> polyData) override;

  vtkSmartPointer<vtkPolyData> ReadPolyData(const char *filename) override;
};

class VTPMeshIODelegate : public AbstractMeshIODelegate
{
public:
  VTPMeshIODelegate();
  virtual ~VTPMeshIODelegate() = default;

  void LoadPolyData(const char *filename, vtkSmartPointer<vtkPolyData> polyData) override;

  vtkSmartPointer<vtkPolyData> ReadPolyData(const char *filename) override;
};

#endif // MESHIODELEGATES_H