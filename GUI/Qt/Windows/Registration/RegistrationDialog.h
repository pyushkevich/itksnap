#ifndef REGISTRATIONDIALOG_H
#define REGISTRATIONDIALOG_H

#include <QDialog>
#include <SNAPCommon.h>

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

private:
  Ui::RegistrationDialog *ui;

  SmartPtr<RegistrationModel> m_Model;
};

#endif // REGISTRATIONDIALOG_H
