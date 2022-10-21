#ifndef DEFORMATIONGRIDRENDERER_H
#define DEFORMATIONGRIDRENDERER_H

#include "GenericSliceRenderer.h"
#include "GenericSliceContextItem.h"
#include "DeformationGridModel.h"

class vtkPoints2D;

class DeformationGridContextItem : public GenericSliceContextItem
{
public:
  vtkTypeMacro(DeformationGridContextItem, GenericSliceContextItem)
  static DeformationGridContextItem *New();

  irisGetSetMacro(DeformationGridModel, DeformationGridModel*)
  irisGetSetMacro(ImageLayer, ImageWrapperBase*)

  virtual bool Paint(vtkContext2D *painter) override;

protected:
  DeformationGridContextItem();
  virtual ~DeformationGridContextItem() {}

  static inline void AddLine(vtkPoints2D *pv, std::vector<double> &verts,
                             size_t skip, size_t l, size_t nv, bool reverse);

  DeformationGridModel *m_DeformationGridModel;

  ImageWrapperBase *m_ImageLayer;
};


class DeformationGridRenderer : public SliceRendererDelegate
{
public:
  irisITKObjectMacro(DeformationGridRenderer, SliceRendererDelegate)

  irisGetSetMacro(Model, DeformationGridModel *)

  virtual void AddContextItemsToTiledOverlay(
      vtkAbstractContextItem *parent, ImageWrapperBase *) override;

protected:
  DeformationGridRenderer();
  virtual ~DeformationGridRenderer() {}

  DeformationGridModel *m_Model;
};

#endif // DEFORMATIONGRIDRENDERER_H
