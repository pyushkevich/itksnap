#ifndef CONTRASTINSPECTOR_H
#define CONTRASTINSPECTOR_H

#include "SNAPComponent.h"

class IntensityCurveModel;
class QtViewportReporter;

namespace Ui {
  class ContrastInspector;
}

class ContrastInspector : public SNAPComponent
{
  Q_OBJECT

public:
  explicit ContrastInspector(QWidget *parent = 0);
  ~ContrastInspector();

  void SetModel(IntensityCurveModel *model);


private slots:
  void on_btnRemoveControl_clicked();

  void on_btnAddControl_clicked();

  void on_btnReset_clicked();

  // Slot for model updates
  void onModelUpdate(const EventBucket &b);


  void on_btnAuto_clicked();

private:

  IntensityCurveModel *m_Model;

  Ui::ContrastInspector *ui;

  // Viewport reporter for the curve box
  QtViewportReporter *m_CurveBoxViewportReporter;
};

#endif // CONTRASTINSPECTOR_H
