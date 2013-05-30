#include "GMMTableModel.h"
#include "SnakeWizardModel.h"
#include "QtCheckBoxCoupling.h"
#include "QtSpinBoxCoupling.h"
#include <QCloseEvent>
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "UnsupervisedClustering.h"
#include "GaussianMixtureModel.h"
#include "ImageWrapperBase.h"
#include "SNAPQtCommon.h"

/* ============================================
 * Qt MODEL Behind the Cluster Listing Table
 * ============================================ */





void GMMTableModel::SetParentModel(SnakeWizardModel *parent)
{
  m_ParentModel = parent;

  LatentITKEventNotifier::connect(m_ParentModel,
                                  SnakeWizardModel::GMMModifiedEvent(),
                                  this, SLOT(onMixtureModelChange(const EventBucket &)));

  LatentITKEventNotifier::connect(m_ParentModel,
                                  itk::DeleteEvent(),
                                  this, SLOT(onMixtureModelChange(const EventBucket &)));
}

void GMMTableModel::onMixtureModelChange(const EventBucket &b)
{
  if(b.HasEvent(itk::DeleteEvent()))
    m_ParentModel = NULL;

  this->layoutChanged();
}

GaussianMixtureModel *GMMTableModel::GetGMM() const
{
  if(!m_ParentModel)
    return NULL;

  // Get the unsupervised clustering class
  UnsupervisedClustering *uc =
    m_ParentModel->GetParent()->GetDriver()->GetClusteringEngine();

  // If we are not in GMM mode, there is no uc!
  if(!uc) return NULL;

  // Get the number of clusters
  return uc->GetMixtureModel();
}

int GMMTableModel::rowCount(const QModelIndex &parent) const
{
  // Get the number of clusters
  GaussianMixtureModel *gmm = this->GetGMM();
  return gmm ? gmm->GetNumberOfGaussians() : 0;
}

int GMMTableModel::columnCount(const QModelIndex &parent) const
{
  // Get the number of clusters
  GaussianMixtureModel *gmm = this->GetGMM();
  return gmm ? gmm->GetNumberOfComponents() + 3 : 0;
}

QVariant GMMTableModel::data(const QModelIndex &index, int role) const
{
  // Get the current row (cluster index)
  int cluster = index.row();
  Column ctype = columnType(index);

  // Get a pointer to the GMM
  GaussianMixtureModel *gmm = this->GetGMM();
  assert(gmm);

  // For first row, return checkboxes
  if(role == Qt::CheckStateRole && ctype == COLUMN_PRIMARY)
    {
    return gmm->IsForeground(cluster) ? Qt::Checked : Qt::Unchecked;
    }

  else if((role == Qt::DisplayRole || role == Qt::EditRole) && ctype == COLUMN_WEIGHT)
    {
    double weight = gmm->GetWeight(cluster);
    return QString::number(weight, 'f', 2);
    }

  else if((role == Qt::DisplayRole || role == Qt::EditRole) && ctype == COLUMN_MEAN)
    {
    double mean = m_ParentModel->GetClusterNativeMean(cluster, columnIndexInType(index));
    return QString("%1").arg(mean, 8, 'g', -1);
    }

  else if((role == Qt::DisplayRole || role == Qt::EditRole)  && ctype == COLUMN_TRACE)
    {
    double var = m_ParentModel->GetClusterNativeTotalVariance(cluster);
    return QString("%1").arg(var, 8, 'g', -1);
    }

  else if(role == Qt::TextAlignmentRole && ctype == COLUMN_PRIMARY)
    {
    return Qt::AlignCenter;
    }

  else if(role == Qt::TextAlignmentRole)
    {
    return Qt::AlignRight;
    }

  else return QVariant();
}

Qt::ItemFlags GMMTableModel::flags(const QModelIndex &index) const
{
  Qt::ItemFlags f = QAbstractTableModel::flags(index);
  switch(columnType(index))
    {
    case COLUMN_PRIMARY:
      return Qt::ItemIsUserCheckable | f;
    case COLUMN_WEIGHT:
      return Qt::ItemIsEditable | f;
    case COLUMN_MEAN:
      return Qt::ItemIsEditable | f;
    default:
      return f;
    }
}

QVariant GMMTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{

  if(orientation == Qt::Horizontal)
    {
    if(columnType(section) == COLUMN_PRIMARY)
      {
      if(role == Qt::DisplayRole)
        return "Foreground";
      else if(role == Qt::ToolTipRole)
        return
            "<p>Use this column to tag clusters as foreground or background.</p> "
            "<p>Foreground clusters are added to the speed function; background "
            "clusters are subtracted from the speed function.</p>";
      }
    else if(columnType(section) == COLUMN_WEIGHT)
      {
      if(role == Qt::DisplayRole)
        return "Weight";
      else if(role == Qt::ToolTipRole)
        return
            "<p>The relative weight of the cluster in the Gaussian mixture</p>";
      }
    else if(columnType(section) == COLUMN_TRACE)
      {
      if(role == Qt::DisplayRole)
        return "Variance";
      else if(role == Qt::ToolTipRole)
        return
            "<p>The overall variance of the cluster</p>"
            "<p>Larger variance values result in smoother clusters</p>";
      }
    else if(columnType(section) == COLUMN_MEAN)
      {
      if(role == Qt::DisplayRole)
        return QString("%1[%2]").arg((QChar) 0x03BC).arg(section-1);
      else if(role == Qt::ToolTipRole)
        {
        // Get the component information
        SnakeWizardModel::ComponentInfo ci =
            m_ParentModel->GetLayerAndIndexForNthComponent(section-COLUMN_MEAN);
        QString nick = from_utf8(ci.ImageWrapper->GetNickname());
        return QString("<html><body>Cluster mean for image <b>%1</b>"
                       " component <b>%2</b></body></html>").arg(nick).arg(ci.ComponentIndex);
        }
      }
    }

  else if(orientation == Qt::Vertical)
    {
    if(role == Qt::DisplayRole)
      return QString("Cluster %1").arg(section+1);
    else if(role == Qt::DecorationRole)
      {
      Vector3d rgb = ColorLabelTable::GetDefaultColorLabel(section+1).GetRGBAsDoubleVector();
      return CreateColorBoxIcon(16, 16, to_unsigned_int(rgb * 255.0));
      }
    }

  return QVariant();
}

bool GMMTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  GaussianMixtureModel *gmm = this->GetGMM();
  int ngauss = gmm->GetNumberOfGaussians()-1;

  if(columnType(index) == COLUMN_PRIMARY)
    {
    bool state = value.toBool();

    // TODO: it would be nice to do this in the mixture model class, without having to
    // have a special method in the SpeedWizardModel class
    if(m_ParentModel->SetClusterForegroundState(index.row(), state))
      {
      emit dataChanged(this->index(0,index.column()), this->index(ngauss, index.column()));
      return true;
      }
    }
  else if(columnType(index) == COLUMN_WEIGHT)
    {
    double weight = value.toDouble();
    if(m_ParentModel->SetClusterWeight(index.row(), weight))
      {
      emit dataChanged(this->index(0,index.column()), this->index(ngauss, index.column()));
      return true;
      }
    }

  else if(columnType(index) == COLUMN_MEAN)
    {
    double mean = value.toDouble();
    if(m_ParentModel->SetClusterNativeMean(
         index.row(), columnIndexInType(index), mean))
      {
      emit dataChanged(index, index);
      return true;
      }
    }

  return false;
}


GMMTableModel::GMMTableModel(QObject *parent)
  : QAbstractTableModel(parent)
{
  m_ParentModel = NULL;
}

GMMTableModel::Column GMMTableModel::columnType(int k) const
{
  GaussianMixtureModel *gmm = this->GetGMM();
  if(!gmm)
    return COLUMN_NONE;

  int n = gmm->GetNumberOfComponents();

  if(k == 0)
    return COLUMN_PRIMARY;
  else if(k == 1)
    return COLUMN_WEIGHT;
  else if(k >= 2 && k < 2 + n)
    return COLUMN_MEAN;
  else if (k == 2 + n)
    return COLUMN_TRACE;
  else
    return COLUMN_NONE;
}

int GMMTableModel::columnIndexInType(int k) const
{
  if(columnType(k) == COLUMN_MEAN)
    return k - 2;
  else
    return 0;
}



GMMItemDelegate::GMMItemDelegate(QObject *parent)
  : QItemDelegate(parent)
{

}

GMMTableModel::Column GMMItemDelegate::columnType(const QModelIndex &index) const
{
  const GMMTableModel *model = dynamic_cast<const GMMTableModel *>(index.model());
  return model->columnType(index);
}


QWidget *GMMItemDelegate
::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{

  if(columnType(index) == GMMTableModel::COLUMN_WEIGHT)
    {
    QDoubleSpinBox *editor = new QDoubleSpinBox(parent);
    editor->setMinimum(0.0);
    editor->setMaximum(1.0);
    editor->setSingleStep(0.01);
    editor->setDecimals(2);

    return editor;
    }
  else
    {
    return QItemDelegate::createEditor(parent, option, index);
    }
}

void
GMMItemDelegate
::setEditorData(QWidget *editor, const QModelIndex &index) const
{
  if(columnType(index) == GMMTableModel::COLUMN_WEIGHT)
    {
    double value = index.model()->data(index, Qt::EditRole).toDouble();
    QDoubleSpinBox *spinBox = static_cast<QDoubleSpinBox*>(editor);
    spinBox->setValue(value);
    }
  else
    {
    QItemDelegate::setEditorData(editor, index);
    }
}

void GMMItemDelegate
::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
  if(columnType(index) == GMMTableModel::COLUMN_WEIGHT)
    {
    QDoubleSpinBox *spinBox = static_cast<QDoubleSpinBox*>(editor);
    spinBox->interpretText();
    double value = spinBox->value();
    model->setData(index, value, Qt::EditRole);
    }
  else
    {
    QItemDelegate::setModelData(editor, model, index);
    }
}

void GMMItemDelegate
::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  if(columnType(index) == GMMTableModel::COLUMN_WEIGHT)
    {
    editor->setGeometry(option.rect);
    }
  else
    {
    QItemDelegate::updateEditorGeometry(editor, option, index);
    }
}
