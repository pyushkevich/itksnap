#ifndef DEFORMATIONGRIDMODEL_H
#define DEFORMATIONGRIDMODEL_H

#include "AbstractModel.h"
#include "SNAPCommon.h"
#include "GenericSliceModel.h"
#include "DisplayMappingPolicy.h"

/** This struct contains vertices' coordinates for a deformation grid.
 *  nline and nvert define the dimension of the grid */
struct DeformationGridVertices
{
  /** Vertice points are packed in x1, y1, x2, y2, .... format */
  std::vector<double> vvec;

  /** Index 0 represents horizontal lines, index 1 represens vertical lines
   *  Dimension of the lines are defined by number of lines (nline),
   *  and number of vertices on each line (nvert) */
  size_t nline[2] = {0l, 0l}, nvert[2] = {0l, 0l};
};

class DeformationGridModel : public AbstractModel
{
public:
  irisITKObjectMacro(DeformationGridModel, AbstractModel)
  irisGetSetMacro(Parent, GenericSliceModel*)

  typedef SliceViewportLayout::SubViewport ViewportType;

  int GetVertices(ImageWrapperBase *layer, DeformationGridVertices &v) const;

protected:
  DeformationGridModel();
  virtual ~DeformationGridModel() {}

  int m_Id; // Identify the view this object being related to
  GenericSliceModel *m_Parent;
};

#endif // DEFORMATIONGRIDMODEL_H
