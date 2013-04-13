#ifndef RESAMPLEDIALOG_H
#define RESAMPLEDIALOG_H

#include <QDialog>

namespace Ui {
class ResampleDialog;
}

class ResampleDialog : public QDialog
{
  Q_OBJECT
  
public:
  explicit ResampleDialog(QWidget *parent = 0);
  ~ResampleDialog();
  
private:
  Ui::ResampleDialog *ui;
};

#endif // RESAMPLEDIALOG_H
