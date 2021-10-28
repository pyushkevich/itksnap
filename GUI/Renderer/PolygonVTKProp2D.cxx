#include "PolygonVTKProp2D.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkQuad.h"

vtkStandardNewMacro(PolygonVTKProp2D)



void PolygonVTKProp2D::SetShapeToRectangle(double x0, double y0, double x1, double y1)
{
  m_PolyData->GetPoints()->Reset();
  m_PolyData->GetPoints()->SetNumberOfPoints(4);
  m_PolyData->GetPoints()->SetPoint(0, x0, y0, 0.0);
  m_PolyData->GetPoints()->SetPoint(0, x0, y1, 0.0);
  m_PolyData->GetPoints()->SetPoint(0, x1, y1, 0.0);
  m_PolyData->GetPoints()->SetPoint(0, x1, y0, 0.0);

  vtkNew<vtkQuad> quad;
  quad->GetPointIds()->SetId(0, 0);
  quad->GetPointIds()->SetId(1, 1);
  quad->GetPointIds()->SetId(2, 2);
  quad->GetPointIds()->SetId(3, 3);

  m_PolyData->GetPolys()->Reset();
  m_PolyData->GetPolys()->InsertNextCell(quad);
}

PolygonVTKProp2D::PolygonVTKProp2D()
{
  vtkNew<vtkPoints> points;
  vtkNew<vtkCellArray> cells;

  m_PolyData = vtkSmartPointer<vtkPolyData>::New();
  m_PolyData->SetPoints(points);
  m_PolyData->SetPolys(cells);

  m_Mapper = vtkSmartPointer<vtkPolyDataMapper2D>::New();
  m_Mapper->SetInputData(m_PolyData);

  this->SetMapper(m_Mapper);
}

PolygonVTKProp2D::~PolygonVTKProp2D()
{

}
