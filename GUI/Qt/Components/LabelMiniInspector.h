#ifndef LABELMINIINSPECTOR_H
#define LABELMINIINSPECTOR_H

#include "SNAPComponent.h"

namespace Ui {
class LabelMiniInspector;
}

class LabelMiniInspector : public SNAPComponent
{
  Q_OBJECT

public:
  explicit LabelMiniInspector(QWidget *parent = 0);
  ~LabelMiniInspector();

  // Set the model
  void SetModel(GlobalUIModel *model);

public slots:

private slots:

private:

  Ui::LabelMiniInspector *ui;

  GlobalUIModel *m_Model;

};

#endif // LABELMINIINSPECTOR_H
