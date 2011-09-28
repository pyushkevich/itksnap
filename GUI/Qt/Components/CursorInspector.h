#ifndef CURSORINSPECTOR_H
#define CURSORINSPECTOR_H

#include "SNAPComponent.h"

class VoxelIntensityQTableModel;


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

protected:

  void UpdateUIFromModel();

private:
  Ui::CursorInspector *ui;
  VoxelIntensityQTableModel *m_TableModel;
  GlobalUIModel *m_Model;
};

#endif // CURSORINSPECTOR_H
