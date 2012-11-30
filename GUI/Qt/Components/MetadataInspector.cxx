#include "MetadataInspector.h"
#include "ui_MetadataInspector.h"
#include <QAbstractTableModel>
#include <QtLineEditCoupling.h>
#include "ImageInfoModel.h"

void MetadataTableQtModel::SetParentModel(ImageInfoModel *model)
{
  m_ParentModel = model;
  LatentITKEventNotifier::connect(
        m_ParentModel,ImageInfoModel::MetadataChangeEvent(),
        this, SLOT(onModelUpdate()));
}

int MetadataTableQtModel::rowCount(const QModelIndex &parent) const
{
  return m_ParentModel->GetMetadataRows();
}

int MetadataTableQtModel::columnCount(const QModelIndex &parent) const
{
  return 2;
}

QVariant MetadataTableQtModel::headerData(
    int section, Qt::Orientation orientation, int role) const
{
  const char *header[] = {"Key", "Value"};
  if(role == Qt::DisplayRole)
    {
    return header[section];
    }
  else return QVariant();
}

QVariant MetadataTableQtModel::data(
    const QModelIndex &index, int role) const
{
  // Ignore bad requests
  if(index.isValid() && (role == Qt::DisplayRole || role == Qt::ToolTipRole))
    return m_ParentModel->GetMetadataCell(index.row(), index.column()).c_str();
  else return QVariant();
}

void MetadataTableQtModel::onModelUpdate()
{
  m_ParentModel->Update();
  emit layoutChanged();
}





MetadataInspector::MetadataInspector(QWidget *parent) :
  SNAPComponent(parent),
  ui(new Ui::MetadataInspector)
{
  ui->setupUi(this);
  m_TableModel = new MetadataTableQtModel(this);
}

MetadataInspector::~MetadataInspector()
{
  delete ui;
}

void MetadataInspector::SetModel(ImageInfoModel *model)
{
  m_Model = model;
  m_TableModel->SetParentModel(model);

  // Hook the table model to the table widget
  ui->tblMetaData->setModel(m_TableModel);

  // Connect the filter input to the model
  makeCoupling(ui->inFilter, m_Model->GetMetadataFilterModel());
}

void MetadataInspector::onModelUpdate(const EventBucket &bucket)
{
}
