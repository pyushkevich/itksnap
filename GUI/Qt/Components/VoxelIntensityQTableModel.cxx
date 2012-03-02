#include "VoxelIntensityQTableModel.h"
#include <GlobalUIModel.h>
#include <IRISApplication.h>
#include <GenericImageData.h>
#include "LatentITKEventNotifier.h"
#include "SNAPEvents.h"

#include "LayerSelectionModel.h"

VoxelIntensityQTableModel::VoxelIntensityQTableModel(QObject *parent) :
    QAbstractTableModel(parent)
{
}

void VoxelIntensityQTableModel::SetParentModel(GlobalUIModel *model)
{
  m_Model = model;

  // Listen to changes in the model
  LatentITKEventNotifier::connect(m_Model, CursorUpdateEvent(),
                                  this, SLOT(onModelUpdate(const EventBucket &)));

  // Listen to changes in the model
  LatentITKEventNotifier::connect(m_Model, LayerChangeEvent(),
                                  this, SLOT(onModelUpdate(const EventBucket &)));
}

int VoxelIntensityQTableModel::rowCount(const QModelIndex &parent) const
{
  return m_Model->GetLoadedLayersSelectionModel()->GetNumberOfLayers();
}

int VoxelIntensityQTableModel::columnCount(const QModelIndex &parent) const
{
  return 2;
}

QVariant VoxelIntensityQTableModel::data(const QModelIndex &index, int role) const
{
  if (role == Qt::DisplayRole)
    {
    // Get the corresponding layer
    LayerIterator it =
        m_Model->GetLoadedLayersSelectionModel()->GetNthLayer(index.row());

    if(index.column() == 0)
      {
      return QString(it.GetDynamicNickname().c_str());
      }
    else
      {
      // Get the cursor position
      Vector3ui cursor = m_Model->GetDriver()->GetCursorPosition();

      // See if this layer is grey
      if(GreyImageWrapperBase *giw = it.GetLayerAsGray())
        {
        return giw->GetVoxelMappedToNative(cursor);
        }
      else if(RGBImageWrapperBase *rgbiw = it.GetLayerAsRGB())
        {
        double rgb[3];
        rgbiw->GetVoxelAsDouble(cursor, rgb);
        return QString("%1,%2,%3").arg(rgb[0]).arg(rgb[1]).arg(rgb[2]);
        }
      }
    }
  return QVariant();
}

QVariant VoxelIntensityQTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role == Qt::DisplayRole)
    {
    if (orientation == Qt::Horizontal)
      {
      return section == 0 ? "Layer" : "Intensity";
      }
    }
  return QVariant();
}

void VoxelIntensityQTableModel::onModelUpdate(const EventBucket &b)
{
  if(b.HasEvent(LayerChangeEvent()))
    {
    this->beginResetModel();
    this->endResetModel();
    }
  else
    {
    int nr = rowCount();
    if(nr > 0)
      {
      this->dataChanged(index(0,1), index(nr-1, 1));
      }
    }
}




