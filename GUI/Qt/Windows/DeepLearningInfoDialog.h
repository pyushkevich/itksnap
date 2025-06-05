#ifndef DEEPLEARNINGINFODIALOG_H
#define DEEPLEARNINGINFODIALOG_H

#include <QDialog>

namespace Ui
{
class DeepLearningInfoDialog;
}

class DeepLearningInfoDialog : public QDialog
{
  Q_OBJECT

public:
  explicit DeepLearningInfoDialog(QWidget *parent = nullptr);
  ~DeepLearningInfoDialog();

  bool isDoNotShowAgainChecked() const;

private slots:
  void on_btnYes_clicked();
  void on_btnNo_clicked();

private:
  Ui::DeepLearningInfoDialog *ui;
};

#endif // DEEPLEARNINGINFODIALOG_H
