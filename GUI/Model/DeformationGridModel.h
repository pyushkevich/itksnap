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

struct DeformationGridVertices
{
  std::list<DeformationGridVertex> vlist[2];

  std::vector<double> vvec;
  float *vts0; // horizontal vertices
  float *vts1; // vertical vertices
  size_t d0_nline, d0_nvert, d1_nline, d1_nvert;
};

class DeformationGridModel : public AbstractModel
{
public:
  irisITKObjectMacro(DeformationGridModel, AbstractModel)
  irisGetSetMacro(Parent, GenericSliceModel*)

  typedef SliceViewportLayout::SubViewport ViewportType;

  int GetVertices(const ViewportType &vp, DeformationGridVertices &v) const;

protected:
  DeformationGridModel();
  virtual ~DeformationGridModel() {}

  int m_Id; // Identify the view this object being related to
  GenericSliceModel *m_Parent;
};

#endif // DEFORMATIONGRIDMODEL_H
