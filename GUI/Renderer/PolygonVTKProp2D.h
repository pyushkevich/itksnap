#ifndef POLYGONVTKPROP2D_H
#define POLYGONVTKPROP2D_H

#include <vtkActor2D.h>
#include <vtkSmartPointer.h>

class vtkPolyData;
class vtkPolyDataMapper2D;

/**
 * @brief Allows quick creation of a rectangle, circle or another 2D polygon.
 *
 *
 */
class PolygonVTKProp2D : public vtkActor2D
{
public:
  vtkTypeMacro(PolygonVTKProp2D, vtkActor2D);

  static PolygonVTKProp2D* New();

  void SetShapeToRectangle(double x0, double y0, double x1, double y1);

protected:
  PolygonVTKProp2D();
  ~PolygonVTKProp2D() override;

  vtkSmartPointer<vtkPolyData> m_PolyData;
  vtkSmartPointer<vtkPolyDataMapper2D> m_Mapper;

private:
  PolygonVTKProp2D(const PolygonVTKProp2D&) = delete;
  void operator=(const PolygonVTKProp2D&) = delete;
};

#endif // POLYGONVTKPROP2D_H
