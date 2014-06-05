#ifndef SPEEDIMAGEDIALOG_H
#define SPEEDIMAGEDIALOG_H

#include <QDialog>
#include <QAbstractTableModel>
#include <SNAPCommon.h>

class SnakeWizardModel;
class ThresholdSettingsRenderer;
class EdgePreprocessingSettingsRenderer;
class GaussianMixtureModel;
class GMMTableModel;
class GMMRenderer;
class ImageLayerSelectionModel;


namespace Ui {
class SpeedImageDialog;
}



class SpeedImageDialog : public QDialog
{
  Q_OBJECT

public:
  explicit SpeedImageDialog(QWidget *parent = 0);
  ~SpeedImageDialog();

  void SetModel(SnakeWizardModel *model);

  /** Make the appropriate pages available based on the state of the model
    and show the dialog */
  void ShowDialog();

  void closeEvent(QCloseEvent *event);

private slots:

  void on_btnClose_clicked();

  void on_btnReinitialize_clicked();

  void on_btnIterate_clicked();

  void on_btnIterateTen_clicked();

  void on_btnTrain_clicked();

private:
  Ui::SpeedImageDialog *ui;

  SnakeWizardModel *m_Model;

  // Renderer used in the threshold settings window.
  // TODO: it seems wrong to have the Qt classes own renderer objects,
  // perhaps this should be owned by the model instead?
  SmartPtr<ThresholdSettingsRenderer> m_ThresholdRenderer;

  SmartPtr<EdgePreprocessingSettingsRenderer> m_EdgeSettingsRenderer;

  SmartPtr<GMMRenderer> m_GMMRenderer;

  // Qt table model for the cluster table
  GMMTableModel *m_GMMTableModel;
};

#endif // SPEEDIMAGEDIALOG_H
