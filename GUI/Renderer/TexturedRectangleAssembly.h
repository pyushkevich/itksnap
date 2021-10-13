#ifndef TEXTUREDRECTANGLEASSEMBLY_H
#define TEXTUREDRECTANGLEASSEMBLY_H

#include <vtkObject.h>
#include <vtkSmartPointer.h>

class vtkActor;
class vtkTexturedActor2D;
class vtkPolyDataMapper;
class vtkPolyDataMapper2D;
class vtkPolyData;

/**
 * @brief Abstract base class for textured rectangle assemblies.
 *
 * This class encapsulates a rectangle in XY plane with texture coords.
 */
class TexturedRectangleAssemblyBase : public vtkObject
{
public:

  vtkTypeMacro(TexturedRectangleAssemblyBase, vtkObject)

  void SetCorners(double x0, double y0, double x1, double y1);

protected:

  TexturedRectangleAssemblyBase();

  vtkSmartPointer<vtkPolyData> m_PolyData;

};

class TexturedRectangleAssembly : public TexturedRectangleAssemblyBase
{
public:
  vtkTypeMacro(TexturedRectangleAssembly, TexturedRectangleAssemblyBase)
  static TexturedRectangleAssembly *New();

  vtkActor *GetActor() const { return m_Actor; }

protected:

  TexturedRectangleAssembly();

  vtkSmartPointer<vtkPolyDataMapper> m_Mapper;
  vtkSmartPointer<vtkActor> m_Actor;
};


class TexturedRectangleAssembly2D : public TexturedRectangleAssemblyBase
{
public:
  vtkTypeMacro(TexturedRectangleAssembly2D, TexturedRectangleAssemblyBase)
  static TexturedRectangleAssembly2D *New();

  vtkTexturedActor2D *GetActor() const { return m_Actor; }

protected:

  TexturedRectangleAssembly2D();

  vtkSmartPointer<vtkPolyDataMapper2D> m_Mapper;
  vtkSmartPointer<vtkTexturedActor2D> m_Actor;
};


#endif // TEXTUREDRECTANGLEASSEMBLY_H
