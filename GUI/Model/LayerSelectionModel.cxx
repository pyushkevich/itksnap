#include "LayerSelectionModel.h"

#include "GlobalUIModel.h"
#include "IRISApplication.h"

int LayerSelectionModel::GetNumberOfLayers()
{
  GenericImageData *id = m_Parent->GetDriver()->GetCurrentImageData();
  return id->GetNumberOfLayers(m_RoleFilter);
}

LayerIterator
LayerSelectionModel::GetNthLayer(int n)
{
  GenericImageData *id = m_Parent->GetDriver()->GetCurrentImageData();
  LayerIterator it(id, m_RoleFilter);
  it += n;
  return it;
}



