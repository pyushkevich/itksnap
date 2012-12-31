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

private:

  Ui::LabelInspector *ui;

  GlobalUIModel *m_Model;

};

#endif // LABELINSPECTOR_H
