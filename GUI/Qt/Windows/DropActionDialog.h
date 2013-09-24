#ifndef DROPACTIONDIALOG_H
#define DROPACTIONDIALOG_H

#include <QDialog>

class GlobalUIModel;
class AbstractLoadImageDelegate;

namespace Ui {
class DropActionDialog;
}



/**
 * This dialog is shown when the user drops an image onto the SNAP window.
 * This dialog is so simple that we don't couple it with its own model, and
 * allow it to hold its data (the dropped filename) in the widgets.
 */
class DropActionDialog : public QDialog
{
  Q_OBJECT
  
public:
  explicit DropActionDialog(QWidget *parent = 0);
  ~DropActionDialog();

  void SetDroppedFilename(QString name);

  void SetModel(GlobalUIModel *model);
  
private slots:
  void on_btnLoadMain_clicked();

  void on_btnLoadSegmentation_clicked();

  void on_btnLoadOverlay_clicked();

  void on_btnLoadNew_clicked();


private:
  Ui::DropActionDialog *ui;
  GlobalUIModel *m_Model;

  void LoadCommon(AbstractLoadImageDelegate *delegate);
};

#endif // DROPACTIONDIALOG_H
