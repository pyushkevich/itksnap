#include "VTKRenderGeometry.h"
#include <vtkPoints2D.h>
#include <vnl_math.h>

vtkSmartPointer<vtkPoints2D> VTKRenderGeometry::MakeUnitCircle(unsigned int n_pts)
{
  return MakeArc(n_pts);
}

vtkSmartPointer<vtkPoints2D> VTKRenderGeometry::MakeUnitDisk(unsigned int n_pts)
{
  return MakeWedge(n_pts);
}

vtkSmartPointer<vtkPoints2D>
VTKRenderGeometry
::MakeWedge(unsigned int n_pts, double a0, double a1, double r_inner, double r_outer)
{
  vtkNew<vtkPoints2D> quad_strip;
  quad_strip->Allocate(2 * n_pts + 2);
  for(unsigned int i = 0; i <= n_pts; i++)
    {
    double t = i * 1.0 / n_pts;
    double alpha = a1 * t + a0 * (1-t);
    double x = cos(alpha), y = sin(alpha);

    quad_strip->InsertNextPoint(r_outer * x, r_outer * y);
    quad_strip->InsertNextPoint(r_inner * x, r_inner * y);
    }

  return quad_strip;
}

vtkSmartPointer<vtkPoints2D>
VTKRenderGeometry
::MakeArc(unsigned int n_pts, double a0, double a1, double r)
{
  vtkNew<vtkPoints2D> poly;
  poly->Allocate(n_pts + 1);
  for(unsigned int i = 0; i <= n_pts; i++)
    {
    double t = i * 1.0 / n_pts;
    double alpha = a1 * t + a0 * (1-t);
    double x = cos(alpha), y = sin(alpha);

    poly->InsertNextPoint(r * x, r * y);
    }

  return poly;
}
