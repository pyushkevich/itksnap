#ifndef GENERIC3DRENDERER_H
#define GENERIC3DRENDERER_H

#include "AbstractVTKRenderer.h"
#include <vtkSmartPointer.h>

class Generic3DModel;
class vtkGenericOpenGLRenderWindow;
class vtkRenderer;
class vtkRenderWindow;
class vtkLineSource;
class vtkActor;
class vtkPropAssembly;
class vtkProperty;
class vtkTransform;

class vtkGlyph3D;

class Generic3DRenderer : public AbstractVTKRenderer
{
public:

  irisITKObjectMacro(Generic3DRenderer, AbstractVTKRenderer)

  void paintGL();

  void SetModel(Generic3DModel *model);

  virtual void OnUpdate();

  void ResetView();

protected:
  Generic3DRenderer();
  virtual ~Generic3DRenderer() {}

  Generic3DModel *m_Model;

  // Update the actors and mappings for the renderer
  void UpdateSegmentationMeshAssembly();
  void UpdateSegmentationMeshAppearance();

  // Update the actors representing the axes
  void UpdateAxisRendering();

  // Update the spray paint glyph properties
  void UpdateSprayGlyphAppearanceAndShape();

  // Update the camera
  void UpdateCamera(bool reset);

  // Prop assembly containing all the segmentation meshes
  vtkSmartPointer<vtkPropAssembly> m_MeshAssembly;

  // Line sources for drawing the crosshairs
  vtkSmartPointer<vtkLineSource> m_AxisLineSource[3];
  vtkSmartPointer<vtkActor> m_AxisActor[3];

  // Glyph filter used to render spray paint stuff
  vtkSmartPointer<vtkGlyph3D> m_SprayGlyphFilter;

  // The property controlling the spray paint
  vtkSmartPointer<vtkProperty> m_SprayProperty;

  // The transform applied to spray points
  vtkSmartPointer<vtkTransform> m_SprayTransform;

};

#endif // GENERIC3DRENDERER_H
