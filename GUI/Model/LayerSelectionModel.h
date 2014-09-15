#ifndef LAYERSELECTIONMODEL_H
#define LAYERSELECTIONMODEL_H

#include "AbstractModel.h"
#include "GenericImageData.h"

class GlobalUIModel;
class ImageWrapperBase;

/**
  This model encapsulates a dynamic selection of image layers based on a
  filter. For example, it can be used to create a dynamic list consisting
  of a main image and all overlays.

  By default, all of the roles are included in the filter.
  */
class LayerSelectionModel : public AbstractModel
{
public:

  irisITKObjectMacro(LayerSelectionModel, AbstractModel)

  /** Set the parent model */
  void SetParentModel(GlobalUIModel *parent)
    { m_Parent = parent; }

  /** Set the filter for this model. See GenericImageData::GetLayer for details */
  void SetRoleFilter(int role_filter)
    { m_RoleFilter = role_filter; }

  /**
    Get the number of layers satisfying the filter. This is updated dynamically,
    so it is always correct
    */
  int GetNumberOfLayers();

  /**
    Get an iterator object pointing to the N-th layer
    */
  LayerIterator GetNthLayer(int n);

protected:

  LayerSelectionModel()
    : AbstractModel(), m_Parent(NULL), m_RoleFilter(ALL_ROLES) {}

  virtual ~LayerSelectionModel() {}

  GlobalUIModel *m_Parent;
  int m_RoleFilter;

};

#endif // LAYERSELECTIONMODEL_H
