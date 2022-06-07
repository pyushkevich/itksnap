#ifndef VTKRENDERGEOMETRY_H
#define VTKRENDERGEOMETRY_H

#include <vtkSmartPointer.h>
#include <vnl_math.h>

class vtkPoints2D;

class VTKRenderGeometry
{
public:

  /** Generate the geometry of a unit circle */
  static vtkSmartPointer<vtkPoints2D> MakeUnitCircle(unsigned int n_pts);

  /** Generate the geometry of the interior of a disk */
  static vtkSmartPointer<vtkPoints2D> MakeUnitDisk(unsigned int n_pts);

  /** Generate the geometry of the interior of a disk from a0 to a1 in radians */
  static vtkSmartPointer<vtkPoints2D> MakeWedge(
      unsigned int n_pts,
      double a0 = 0, double a1 = vnl_math::twopi,
      double r_inner = 0.0, double r_outer = 1.0);

  /** Generate the geometry of an outline of a circle from a0 to a1 in radians */
  static vtkSmartPointer<vtkPoints2D> MakeArc(
      unsigned int n_pts,
      double a0 = 0, double a1 = vnl_math::twopi, double r = 1.0);
};


#endif // VTKRENDERGEOMETRY_H
