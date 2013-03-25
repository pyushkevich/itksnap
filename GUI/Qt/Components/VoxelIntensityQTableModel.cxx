#include "VoxelIntensityQTableModel.h"
#include <GlobalUIModel.h>
#include <IRISException.h>
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

#include <iomanip>

QVariant VoxelIntensityQTableModel::data(const QModelIndex &index, int role) const
{
  if (role == Qt::DisplayRole)
    {
    // Get the corresponding layer
    LayerIterator it =
        m_Model->GetLoadedLayersSelectionModel()->GetNthLayer(index.row());

    if(index.column() == 0)
      {
      return QString(it.GetLayer()->GetNickname().c_str());
      }
    else
      {
      // Get the cursor position
      Vector3ui cursor = m_Model->GetDriver()->GetCursorPosition();

      // TODO: do we want to use a tree model here to represent multi-channel
      // images? For the time being, we can list all of the components, but
      // we should really come up with something better
      ImageWrapperBase *iw = it.GetLayer();
      vnl_vector<double> voxel(iw->GetNumberOfComponents(), 0.0);
      iw->GetVoxelMappedToNative(cursor, voxel.data_block());

      if(voxel.size() > 1)
        {
        std::ostringstream oss;
        for(int i = 0; i < voxel.size(); i++)
          {
          if(i > 0)
            oss << ",";
          oss << std::setprecision(3) << voxel[i];
          }
        return QString(oss.str().c_str());
        }
      else
        {
        return voxel[0];
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




