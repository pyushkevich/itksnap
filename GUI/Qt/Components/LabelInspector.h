#ifndef LABELINSPECTOR_H
#define LABELINSPECTOR_H

#include "SNAPComponent.h"

namespace Ui {
class LabelInspector;
}

class LabelInspector : public SNAPComponent
{
  Q_OBJECT

public:
  explicit LabelInspector(QWidget *parent = 0);
  ~LabelInspector();

  // Set the model
  void SetModel(GlobalUIModel *model);

public slots:

private slots:

  void on_actionEdit_Label_triggered();

  void on_actionLocate_Center_of_Mass_triggered();

  void on_actionNew_label_triggered();

private:

  Ui::LabelInspector *ui;

  GlobalUIModel *m_Model;

};

#endif // LABELINSPECTOR_H
