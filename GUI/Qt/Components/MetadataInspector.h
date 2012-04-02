#ifndef METADATAINSPECTOR_H
#define METADATAINSPECTOR_H

#include <SNAPComponent.h>
#include <SNAPCommon.h>
#include <QAbstractTableModel>

class ImageInfoModel;
class MetadataTableQtModel;

namespace Ui {
class MetadataInspector;
}


class MetadataTableQtModel : public QAbstractTableModel
{
  Q_OBJECT

public:
  MetadataTableQtModel(QWidget *parent) : QAbstractTableModel(parent) {}
  virtual ~MetadataTableQtModel() {}

  void SetParentModel(ImageInfoModel *model);
  int rowCount(const QModelIndex &parent) const;
  int columnCount(const QModelIndex &parent) const;

  QVariant headerData(int section, Qt::Orientation orientation, int role) const;

  QVariant data(const QModelIndex &index, int role) const;

public slots:

  void onModelUpdate();


protected:

  ImageInfoModel *m_ParentModel;
};


class MetadataInspector : public SNAPComponent
{
  Q_OBJECT

public:
  explicit MetadataInspector(QWidget *parent = 0);
  ~MetadataInspector();

  // Set the model
  void SetModel(ImageInfoModel *model);

  // Respond to model updates
  virtual void onModelUpdate(const EventBucket &bucket);

private:
  Ui::MetadataInspector *ui;
  MetadataTableQtModel *m_TableModel;
  ImageInfoModel *m_Model;
};

#endif // METADATAINSPECTOR_H
