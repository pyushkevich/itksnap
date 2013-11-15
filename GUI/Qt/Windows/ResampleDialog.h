#ifndef RESAMPLEDIALOG_H
#define RESAMPLEDIALOG_H

#include <QDialog>

namespace Ui {
class ResampleDialog;
}

class QAbstractButton;
class SnakeROIResampleModel;

class ResampleDialog : public QDialog
{
  Q_OBJECT
  
public:
  explicit ResampleDialog(QWidget *parent = 0);
  ~ResampleDialog();

  void SetModel(SnakeROIResampleModel *model);

private slots:
  void on_buttonBox_clicked(QAbstractButton *button);

  void on_actionSuper2_triggered();

  void on_actionSub2_triggered();

  void on_actionSuperIso_triggered();

  void on_actionSubIso_triggered();

private:
  Ui::ResampleDialog *ui;

  SnakeROIResampleModel *m_Model;
};

#endif // RESAMPLEDIALOG_H
