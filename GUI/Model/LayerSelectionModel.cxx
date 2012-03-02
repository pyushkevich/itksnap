#include "LayerSelectionModel.h"

#include "GlobalUIModel.h"
#include "IRISApplication.h"

int LayerSelectionModel::GetNumberOfLayers()
{
  GenericImageData *id = m_Parent->GetDriver()->GetCurrentImageData();
  return id->GetNumberOfLayers(m_RoleFilter);
}

ImageWrapperBase *
LayerSelectionModel::GetNthLayer(int n)
{
  GenericImageData *id = m_Parent->GetDriver()->GetCurrentImageData();
  return id->GetNthLayer(n, m_RoleFilter);
}

GreyImageWrapperBase *
LayerSelectionModel::GetNthLayerAsGrey(int n)
{
  return dynamic_cast<GreyImageWrapperBase *>(this->GetNthLayer(n));
}

RGBImageWrapperBase *
LayerSelectionModel::GetNthLayerAsRGB(int n)
{
  return dynamic_cast<RGBImageWrapperBase *>(this->GetNthLayer(n));
}

LayerIterator::LayerRole
LayerSelectionModel::GetRoleOfNthLayer(int n)
{
  GenericImageData *id = m_Parent->GetDriver()->GetCurrentImageData();
  LayerIterator it(id, m_RoleFilter);
  it += n;
  return it.GetRole();
}



