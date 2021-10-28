#include "TexturedRectangleAssembly.h"

#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkFloatArray.h>
#include <vtkQuad.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkActor.h>
#include <vtkTexturedActor2D.h>
#include <vtkProperty.h>
#include <vtkProperty2D.h>

void TexturedRectangleAssemblyBase::SetCorners(double x0, double y0, double x1, double y1)
{
  m_PolyData->GetPoints()->SetPoint(0, x0, y0, 0.0);
  m_PolyData->GetPoints()->SetPoint(1, x0, y1, 0.0);
  m_PolyData->GetPoints()->SetPoint(2, x1, y1, 0.0);
  m_PolyData->GetPoints()->SetPoint(3, x1, y0, 0.0);
  m_PolyData->GetPoints()->Modified();
}

TexturedRectangleAssemblyBase::TexturedRectangleAssemblyBase()
{
  m_PolyData = vtkSmartPointer<vtkPolyData>::New();
  m_PolyData->SetPoints(vtkSmartPointer<vtkPoints>::New());
  m_PolyData->GetPoints()->InsertNextPoint(0, 0, 0);
  m_PolyData->GetPoints()->InsertNextPoint(0, 100, 0);
  m_PolyData->GetPoints()->InsertNextPoint(100, 100, 0);
  m_PolyData->GetPoints()->InsertNextPoint(0, 0, 0);

  vtkNew<vtkQuad> quad;
  quad->GetPointIds()->SetId(0, 0);
  quad->GetPointIds()->SetId(1, 1);
  quad->GetPointIds()->SetId(2, 2);
  quad->GetPointIds()->SetId(3, 3);
  m_PolyData->SetPolys(vtkSmartPointer<vtkCellArray>::New());
  m_PolyData->GetPolys()->InsertNextCell(quad);

  // Set texture coordinates
  vtkNew<vtkFloatArray> tcoords;
  tcoords->SetNumberOfComponents(2);
  tcoords->InsertNextTuple2(0, 0);
  tcoords->InsertNextTuple2(0, 1);
  tcoords->InsertNextTuple2(1, 1);
  tcoords->InsertNextTuple2(1, 0);
  m_PolyData->GetPointData()->SetTCoords(tcoords);
}

TexturedRectangleAssembly::TexturedRectangleAssembly()
{
  // Create the main image actor
  m_Mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  m_Mapper->SetInputData(m_PolyData);

  m_Actor = vtkSmartPointer<vtkActor>::New();
  m_Actor->SetMapper(m_Mapper);
  m_Actor->GetProperty()->SetColor(1.0, 1.0, 1.0);
}

TexturedRectangleAssembly2D::TexturedRectangleAssembly2D()
{
  // Create the main image actor
  m_Mapper = vtkSmartPointer<vtkPolyDataMapper2D>::New();
  m_Mapper->SetInputData(m_PolyData);

  m_Actor = vtkSmartPointer<vtkTexturedActor2D>::New();
  m_Actor->SetMapper(m_Mapper);
  m_Actor->GetProperty()->SetColor(1.0, 1.0, 1.0);
}

vtkStandardNewMacro(TexturedRectangleAssembly);
vtkStandardNewMacro(TexturedRectangleAssembly2D);
