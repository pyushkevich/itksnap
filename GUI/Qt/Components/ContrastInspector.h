#ifndef CONTRASTINSPECTOR_H
#define CONTRASTINSPECTOR_H

#include "SNAPComponent.h"

class IntensityCurveBox;
class IntensityCurveModel;

namespace Ui {
    class ContrastInspector;
}

class ContrastInspector : public SNAPComponent
{
    Q_OBJECT

public:
    explicit ContrastInspector(QWidget *parent = 0);
    ~ContrastInspector();

  IntensityCurveBox *GetCurveBox();

  void SetModel(IntensityCurveModel *model);


private slots:
  void on_btnRemoveControl_clicked();

  void on_btnAddControl_clicked();

  void on_btnReset_clicked();

  void onModelUpdate(const EventBucket &b);

private:

  IntensityCurveModel *m_Model;

  Ui::ContrastInspector *ui;
};

#endif // CONTRASTINSPECTOR_H
