#ifndef SPEEDIMAGEDIALOG_H
#define SPEEDIMAGEDIALOG_H

#include <QDialog>
#include <QAbstractTableModel>
#include <SNAPCommon.h>

class SnakeWizardModel;
class ThresholdSettingsRenderer;
class EdgePreprocessingSettingsRenderer;
class GaussianMixtureModel;

namespace Ui {
class SpeedImageDialog;
}

// The qt model for the GMM cluster list
class GMMTableModel : public QAbstractTableModel
{
  Q_OBJECT

public:

  GMMTableModel(QObject *parent);

  virtual int rowCount(const QModelIndex &parent) const;

  virtual int columnCount(const QModelIndex &parent) const;

  virtual QVariant data(const QModelIndex &index, int role) const;

  virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;

  void SetParentModel(SnakeWizardModel *parent);

public slots:

  void onMixtureModelChange();

protected:

  SnakeWizardModel *m_ParentModel;


  GaussianMixtureModel *GetGMM() const;


};


class SpeedImageDialog : public QDialog
{
  Q_OBJECT

public:
  explicit SpeedImageDialog(QWidget *parent = 0);
  ~SpeedImageDialog();

  void SetModel(SnakeWizardModel *model);

  /** Make the appropriate pages available based on the state of the model
    and show the dialog */
  void SetPageAndShow();

  void closeEvent(QCloseEvent *event);

private slots:
  void on_btnApply_clicked();

  void on_btnOk_clicked();

  void on_btnClose_clicked();

  void on_tabWidgetEdge_currentChanged(int index);

  void on_tabWidgetInOut_currentChanged(int index);

  void on_btnReinitialize_clicked();

  void on_btnIterate_clicked();

private:
  Ui::SpeedImageDialog *ui;

  SnakeWizardModel *m_Model;

  // Renderer used in the threshold settings window.
  // TODO: it seems wrong to have the Qt classes own renderer objects,
  // perhaps this should be owned by the model instead?
  SmartPtr<ThresholdSettingsRenderer> m_ThresholdRenderer;

  SmartPtr<EdgePreprocessingSettingsRenderer> m_EdgeSettingsRenderer;

  // Qt table model for the cluster table
  GMMTableModel *m_GMMTableModel;
};

#endif // SPEEDIMAGEDIALOG_H
