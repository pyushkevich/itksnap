#ifndef CURSORINSPECTOR_H
#define CURSORINSPECTOR_H

#include "SNAPComponent.h"

class CursorInspectionModel;
class QTableWidgetItem;
class QStandardItemModel;
struct LayerCurrentVoxelInfo;

namespace Ui {
    class CursorInspector;
}

class CursorInspector : public SNAPComponent
{
  Q_OBJECT

public:
  explicit CursorInspector(QWidget *parent = 0);

  void SetModel(CursorInspectionModel *);

  ~CursorInspector();

public slots:

  void onContextMenuRequested(QPoint pos);

protected:

  void RebuildVoxelTable();
  void UpdateVoxelTable();

  void UpdateVoxelTableRow(int i, const LayerCurrentVoxelInfo &vi);
private slots:

  void onModelUpdate(const EventBucket &bucket);

  void on_tableVoxelUnderCursor_clicked(const QModelIndex &index);

private:
  Ui::CursorInspector *ui;

  CursorInspectionModel *m_Model;

  QStandardItemModel *m_CurrentVoxelItemModel;

  int m_PopupRow;
};

#endif // CURSORINSPECTOR_H
