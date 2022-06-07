#ifndef LAYOUTREMINDERDIALOG_H
#define LAYOUTREMINDERDIALOG_H

#include <QDialog>
#include <SNAPCommon.h>

namespace Ui {
  class LayoutReminderDialog;
}

class GlobalUIModel;

class LayoutReminderDialog : public QDialog
{
  Q_OBJECT

public:
  explicit LayoutReminderDialog(QWidget *parent = nullptr);
  ~LayoutReminderDialog();
  void Initialize(GlobalUIModel* model);
  enum ExecScenarios{Default = 0, Echo_Cartesian_Dicom_Loading};
  void ConditionalExec(enum ExecScenarios = Default);

protected:
  void SetReminderFlag();

protected slots:
  void on_btnYes_clicked();
  void on_btnNo_clicked();

private:
  Ui::LayoutReminderDialog *ui;
  SmartPtr<GlobalUIModel> m_GlobalUIModel;
};

#endif // LAYOUTREMINDERDIALOG_H
