#ifndef JOINMODEL_H
#define JOINMODEL_H

#include <SNAPCommon.h>
#include "AbstractModel.h"
#include "GenericSliceModel.h"

class GenericSliceModel;

class JoinModel : public AbstractModel
{
public:
  irisITKObjectMacro(JoinModel, AbstractModel)

  irisGetMacro(Parent, GenericSliceModel *)

  void SetParent(GenericSliceModel *);

  bool ProcessPushEvent(const Vector3f &xSlice, bool reverse_mode, bool ctrl);

  void ProcessLeaveEvent();

protected:

  // Mouse position in voxel coordinates
  Vector3ui m_MousePosition;
  bool m_MouseInside;

  // Private constructor stuff
  JoinModel();
  virtual ~JoinModel() {}

  void ComputeMousePosition(const Vector3f &xSlice);

  void processPickLabel(bool reverse_mode);
  bool processCnJ(bool reverse_mode);

  // Parent model
  GenericSliceModel *m_Parent;

};

#endif // JOINMODEL_H
