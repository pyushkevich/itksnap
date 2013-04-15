#ifndef RESAMPLEDIALOG_H
#define RESAMPLEDIALOG_H

#include <QDialog>

namespace Ui {
class ResampleDialog;
}

class SnakeROIResampleModel;

class ResampleDialog : public QDialog
{
  Q_OBJECT
  
public:
  explicit ResampleDialog(QWidget *parent = 0);
  ~ResampleDialog();

  void SetModel(SnakeROIResampleModel *model);

private:
  Ui::ResampleDialog *ui;

  SnakeROIResampleModel *m_Model;
};

#endif // RESAMPLEDIALOG_H
