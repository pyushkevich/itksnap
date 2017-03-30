#ifndef INTERPOLATELABELSDIALOG_H
#define INTERPOLATELABELSDIALOG_H

#include <QDialog>
#include <SNAPCommon.h>

namespace Ui {
class InterpolateLabelsDialog;
}

class InterpolateLabelModel;

class InterpolateLabelsDialog : public QDialog
{
  Q_OBJECT

public:
  explicit InterpolateLabelsDialog(QWidget *parent = 0);
  ~InterpolateLabelsDialog();

  void SetModel(InterpolateLabelModel *model);

private slots:
  void on_btnInterpolate_clicked();

  void on_btnClose_clicked();

private:
  Ui::InterpolateLabelsDialog *ui;

  SmartPtr<InterpolateLabelModel> m_Model;

  virtual void showEvent(QShowEvent *e);
};

#endif // INTERPOLATELABELSDIALOG_H
