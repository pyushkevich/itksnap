#ifndef CURSORINSPECTOR_H
#define CURSORINSPECTOR_H

#include <QWidget>

class GlobalUIModel;
class VoxelIntensityQTableModel;

namespace Ui {
    class CursorInspector;
}

class CursorInspector : public QWidget
{
  Q_OBJECT

public:
  explicit CursorInspector(QWidget *parent = 0);

  void SetModel(GlobalUIModel *);

  ~CursorInspector();

protected:

  void UpdateUIFromModel();

private:
  Ui::CursorInspector *ui;
  VoxelIntensityQTableModel *m_TableModel;
  GlobalUIModel *m_Model;
};

#endif // CURSORINSPECTOR_H
