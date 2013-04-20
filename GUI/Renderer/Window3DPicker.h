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

  irisIsMacro(PickSuccessful)

  irisGetMacro(PickPosition, Vector3i)

protected:
  virtual double IntersectWithLine(double p1[3], double p2[3], double tol, vtkAssemblyPath *, vtkProp3D *, vtkAbstractMapper3D *);

  Window3DPicker();

  Generic3DModel *m_Model;
  bool m_PickSuccessful;
  Vector3i m_PickPosition;
};



#endif // WINDOW3DPICKER_H
