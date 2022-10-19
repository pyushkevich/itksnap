#ifndef DEFORMATIONGRIDMODEL_H
#define DEFORMATIONGRIDMODEL_H

#include "AbstractModel.h"
#include "SNAPCommon.h"
#include "GenericSliceModel.h"
#include "DisplayMappingPolicy.h"

struct DeformationGridVertex
{
  double x;
  double y;
  DeformationGridVertex();
  DeformationGridVertex(double x, double y);
};

class DeformationGridModel : public AbstractModel
{
public:
  irisITKObjectMacro(DeformationGridModel, AbstractModel)
  irisGetSetMacro(Parent, GenericSliceModel*)

  typedef SliceViewportLayout::SubViewport ViewportType;

  int GetVertices(const ViewportType &vp, std::list<DeformationGridVertex> &list) const;

protected:
  DeformationGridModel();
  virtual ~DeformationGridModel() {}

  int m_Id; // Identify the view this object being related to
  GenericSliceModel *m_Parent;
};

#endif // DEFORMATIONGRIDMODEL_H
