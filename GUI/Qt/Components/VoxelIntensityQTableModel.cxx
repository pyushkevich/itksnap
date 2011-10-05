#include "VoxelIntensityQTableModel.h"
#include <GlobalUIModel.h>
#include <IRISApplication.h>
#include <GenericImageData.h>
#include "LatentITKEventNotifier.h"
#include "SNAPEvents.h"

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
  GenericImageData *gid = m_Model->GetDriver()->GetCurrentImageData();
  return gid->GetNumberOfLayers();
}

int VoxelIntensityQTableModel::columnCount(const QModelIndex &parent) const
{
  return 2;
}

QVariant VoxelIntensityQTableModel::data(const QModelIndex &index, int role) const
{
  IRISApplication *app = m_Model->GetDriver();
  GenericImageData *gid = app->GetCurrentImageData();
  if (role == Qt::DisplayRole)
    {
    if(index.column() == 0)
      {
      if(index.row() == 0)
        return "Main image";
      else
        return QString("Overlay %1").arg(index.row());
      }
    else
      {
      Vector3ui cursor = app->GetCursorPosition();
      if(GreyImageWrapperBase *giw = gid->GetLayerAsGray(index.row()))
        {
        return giw->GetVoxelMappedToNative(cursor);
        }
      else if(RGBImageWrapperBase *rgbiw = gid->GetLayerAsRGB(index.row()))
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




