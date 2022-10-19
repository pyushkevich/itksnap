#ifndef DEFORMATIONGRIDRENDERER_H
#define DEFORMATIONGRIDRENDERER_H

#include "GenericSliceRenderer.h"
#include "GenericSliceContextItem.h"
#include "DeformationGridModel.h"


class DeformationGridContextItem : public GenericSliceContextItem
{
public:
  vtkTypeMacro(DeformationGridContextItem, GenericSliceContextItem)
  static DeformationGridContextItem *New();

  irisGetSetMacro(DeformationGridModel, DeformationGridModel*)

  virtual bool Paint(vtkContext2D *painter) override;

protected:
  DeformationGridContextItem();
  virtual ~DeformationGridContextItem() {}

  DeformationGridModel *m_DeformationGridModel;
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
