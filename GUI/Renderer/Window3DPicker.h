#ifndef WINDOW3DPICKER_H
#define WINDOW3DPICKER_H

#include "vtkPicker.h"
#include "SNAPCommon.h"

class Generic3DModel;

class Window3DPicker : public vtkPicker
{
public:
  static Window3DPicker *New();

  vtkTypeRevisionMacro(Window3DPicker, vtkPicker)

  irisGetSetMacro(Model, Generic3DModel *)


protected:
  virtual double IntersectWithLine(double p1[3], double p2[3], double tol, vtkAssemblyPath *, vtkProp3D *, vtkAbstractMapper3D *);

  Generic3DModel *m_Model;
};



#endif // WINDOW3DPICKER_H
