#ifndef REGISTRATIONDIALOG_H
#define REGISTRATIONDIALOG_H

#include <QDialog>

class RegistrationModel;

namespace Ui {
class RegistrationDialog;
}

class RegistrationDialog : public QDialog
{
  Q_OBJECT

public:
  explicit RegistrationDialog(QWidget *parent = 0);
  ~RegistrationDialog();

  void SetModel(RegistrationModel *model);

private slots:
  void on_pushButton_clicked();

  void on_pushButton_2_clicked();

  void on_btnRunRegistration_clicked();

private:
  Ui::RegistrationDialog *ui;

  RegistrationModel *m_Model;
};

#endif // REGISTRATIONDIALOG_H
