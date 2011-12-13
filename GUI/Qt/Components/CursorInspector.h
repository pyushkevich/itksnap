#ifndef CURSORINSPECTOR_H
#define CURSORINSPECTOR_H

#include "SNAPComponent.h"

class VoxelIntensityQTableModel;
class QMenu;


namespace Ui {
    class CursorInspector;
}

class CursorInspector : public SNAPComponent
{
  Q_OBJECT

public:
  explicit CursorInspector(QWidget *parent = 0);

  void SetModel(GlobalUIModel *);

  ~CursorInspector();

public slots:

  void onCursorEdit();
  void onModelUpdate(const EventBucket &);

  void onContextMenuRequested(QPoint pos);

protected:

  void UpdateUIFromModel();

private slots:
  void on_actionAutoContrast_triggered();

private:
  Ui::CursorInspector *ui;
  VoxelIntensityQTableModel *m_TableModel;
  GlobalUIModel *m_Model;
  QMenu *m_ContextMenu;
};

#endif // CURSORINSPECTOR_H
