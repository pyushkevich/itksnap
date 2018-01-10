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

  /**
   * Attempt to load the main image directly, without showing the dialog,
   * but showing the ImageIOWizard if there is ambiguity about how to load
   * the image
   */
  void LoadMainImage(QString name);

private slots:
  void on_btnLoadMain_clicked();

  void on_btnLoadSegmentation_clicked();

  void on_btnLoadOverlay_clicked();

  void on_btnLoadNew_clicked();

  /**
   * This method loads the dropped image.
   */
  void LoadCommon(AbstractLoadImageDelegate *delegate);


  void on_btnLoadAdditionalSegmentation_clicked();

private:
  Ui::DropActionDialog *ui;
  GlobalUIModel *m_Model;

};

#endif // DROPACTIONDIALOG_H
