#ifndef QTWARNINGDIALOG_H
#define QTWARNINGDIALOG_H

namespace Ui {
class QtWarningDialog;
}

#include "IRISException.h"
#include <list>
#include <QDialog>

class QLabel;
class QCheckBox;
class QPushButton;

class QtWarningDialog : public QDialog
{
  Q_OBJECT

public:

  explicit QtWarningDialog(QWidget *parent = 0);
  ~ QtWarningDialog();

  static void show(const std::vector<IRISWarning> &warnings);

public slots:

private slots:
  void on_pushButton_clicked();

private:
  QLabel *m_Label;
  QPushButton *m_Button;
  QCheckBox *m_Checkbox;

  Ui::QtWarningDialog *ui;
};

#endif // QTWARNINGDIALOG_H
