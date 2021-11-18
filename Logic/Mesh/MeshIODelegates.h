#ifndef MESHIODELEGATES_H
#define MESHIODELEGATES_H

#include "SNAPCommon.h"
#include "IRISApplication.h"
#include "GuidedMeshIO.h"

class MeshWrapperBase;

class AbstractMeshIODelegate : public itk::Object
{
public:
  irisITKAbstractObjectMacro(AbstractMeshIODelegate, itk::Object)

  /** Load mesh data into the out pointer */
  virtual void LoadMesh(const char *filename, vtkPolyData* polyData) = 0;

protected:
  AbstractMeshIODelegate();
  virtual ~AbstractMeshIODelegate() {}
};

class VTKMeshIODelegate : public AbstractMeshIODelegate
{
public:
  irisITKObjectMacro(VTKMeshIODelegate, AbstractMeshIODelegate)

  virtual void LoadMesh(const char *filename, vtkPolyData* polyData) override;

protected:
  VTKMeshIODelegate();
  virtual ~VTKMeshIODelegate() {}
};

class VTPMeshIODelegate : public AbstractMeshIODelegate
{
public:
  irisITKObjectMacro(VTPMeshIODelegate, AbstractMeshIODelegate)

  virtual void LoadMesh(const char *filename, vtkPolyData* polyData) override;

protected:
  VTPMeshIODelegate();
  virtual ~VTPMeshIODelegate() {}
};


/** Produce a MeshIODelegete for specific mesh file format */
class MeshIODelegateFactory : public itk::Object
{
public:
  irisITKObjectMacro(MeshIODelegateFactory, itk::Object)

  /** Get an AbstractMeshIODelegate for the provided mesh file format */
  AbstractMeshIODelegate *GetDelegate(GuidedMeshIO::FileFormat FileFormat);

protected:
  MeshIODelegateFactory();
  virtual ~MeshIODelegateFactory() {}
};



#endif // MESHIODELEGATES_H
