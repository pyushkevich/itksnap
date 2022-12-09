#ifndef DROPACTIONDIALOG_H
#define DROPACTIONDIALOG_H

#include <QDialog>

class GlobalUIModel;
class AbstractOpenImageDelegate;
class EventBucket;

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

  /**
   * Attempt to load a mesh directly without showing the dialog
   */
  void LoadMesh(QString name);

  /**
   * Load a file without Main Image loaded
   * If file is a valid mesh file, load as mesh layer
   * Otherwise call LoadMainImage
   */
  void InitialLoad(QString name);

  /**
   * Whether to include mesh options
   */
  void SetIncludeMeshOptions(bool include_mesh);

private slots:
  void on_btnLoadMain_clicked();

  void on_btnLoadSegmentation_clicked();

  void on_btnLoadOverlay_clicked();

  void on_btnLoadNew_clicked();

  void on_btnLoadMeshAsLayer_clicked();

  void on_btnLoadMeshToTP_clicked();

  void onModelUpdate(const EventBucket &bucket);

  /**
   * This method loads the dropped image.
   */
  void LoadCommon(AbstractOpenImageDelegate *delegate);


  void on_btnLoadAdditionalSegmentation_clicked();

private:
  Ui::DropActionDialog *ui;
  GlobalUIModel *m_Model;

};

#endif // DROPACTIONDIALOG_H
