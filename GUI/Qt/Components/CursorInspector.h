#ifndef CURSORINSPECTOR_H
#define CURSORINSPECTOR_H

#include "SNAPComponent.h"


class QMenu;
class CursorInspectionModel;
class QtScriptTest1;

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


private slots:
  void on_actionAutoContrast_triggered();

private:
  Ui::CursorInspector *ui;
  friend class QtScriptTest1;


  CursorInspectionModel *m_Model;
  QMenu *m_ContextMenu;

  int m_PopupRow;
};

#endif // CURSORINSPECTOR_H
