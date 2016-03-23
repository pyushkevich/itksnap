#ifndef INTERPOLATELABELSDIALOG_H
#define INTERPOLATELABELSDIALOG_H

#include <QDialog>

namespace Ui {
class InterpolateLabelsDialog;
}

class InterpolateLabelsDialog : public QDialog
{
  Q_OBJECT

public:
  explicit InterpolateLabelsDialog(QWidget *parent = 0);
  ~InterpolateLabelsDialog();

private:
  Ui::InterpolateLabelsDialog *ui;
};

#endif // INTERPOLATELABELSDIALOG_H
