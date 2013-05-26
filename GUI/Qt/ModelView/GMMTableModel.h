#ifndef GMMTABLEMODEL_H
#define GMMTABLEMODEL_H

#include <QDialog>
#include <QAbstractTableModel>
#include <QItemDelegate>
#include <SNAPCommon.h>

class SnakeWizardModel;
class ThresholdSettingsRenderer;
class EdgePreprocessingSettingsRenderer;
class GaussianMixtureModel;
class EventBucket;

/**
 * The qt model for the GMM cluster list
 */
class GMMTableModel : public QAbstractTableModel
{
  Q_OBJECT

public:

  /** Classes of columns in the table */
  enum Column {
    COLUMN_PRIMARY,
    COLUMN_TRACE,
    COLUMN_MEAN,
    COLUMN_WEIGHT,
    COLUMN_NONE
  };

  GMMTableModel(QObject *parent);

  Column columnType(int column) const;
  Column columnType(const QModelIndex &index) const
    { return columnType(index.column()); }

  int columnIndexInType(int column) const;
  int columnIndexInType(const QModelIndex &index) const
    { return columnIndexInType(index.column()); }

  virtual int rowCount(const QModelIndex &parent) const;

  virtual int columnCount(const QModelIndex &parent) const;

  virtual QVariant data(const QModelIndex &index, int role) const;

  virtual Qt::ItemFlags flags(const QModelIndex &index) const;

  virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;

  virtual bool setData(const QModelIndex &index, const QVariant &value, int role);

  void SetParentModel(SnakeWizardModel *parent);

public slots:

  void onMixtureModelChange(const EventBucket &);

protected:

  SnakeWizardModel *m_ParentModel;
  GaussianMixtureModel *GetGMM() const;
};

/**
 * The delegate model for the same
 */
class GMMItemDelegate : public QItemDelegate
{
  Q_OBJECT

public:
  GMMItemDelegate(QObject *parent = 0);

  GMMTableModel::Column columnType(const QModelIndex &index) const;

  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                        const QModelIndex &index) const;

  void setEditorData(QWidget *editor, const QModelIndex &index) const;
  void setModelData(QWidget *editor, QAbstractItemModel *model,
                    const QModelIndex &index) const;

  void updateEditorGeometry(QWidget *editor,
                            const QStyleOptionViewItem &option, const QModelIndex &index) const;
};


#endif // GMMTABLEMODEL_H
