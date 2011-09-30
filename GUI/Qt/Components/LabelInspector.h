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

  // Respond to model update events
  void onModelUpdate(const EventBucket &bucket);

private slots:
  void on_inForeLabel_currentIndexChanged(int index);

  void on_inBackLabel_currentIndexChanged(int index);

  void on_inOpacity_valueChanged(int value);

  void on_btnEdit_clicked();

  void on_cbInvert_stateChanged(int arg1);

private:

  static const int ICON_SIZE;

  // Populate the fields from current state
  void FillCombos();
  void UpdateValuesFromModel();

  Ui::LabelInspector *ui;

  GlobalUIModel *m_Model;
};

#endif // LABELINSPECTOR_H
