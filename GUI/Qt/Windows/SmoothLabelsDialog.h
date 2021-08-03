#ifndef SMOOTHLABELSDIALOG_H
#define SMOOTHLABELSDIALOG_H

// issue #24: Add label smoothing feature

#include<QDialog>
#include<SNAPCommon.h>

class SmoothLabelsModel;
class QStandardItemModel;
class QSortFilterProxyModel;

namespace Ui
{
  class SmoothLabelsDialog;
}


class SmoothLabelsDialog : public QDialog
{
  Q_OBJECT

public:
  explicit SmoothLabelsDialog(QWidget *parent = 0);

  ~SmoothLabelsDialog();

  void SetModel(SmoothLabelsModel *model);

private slots:
  void on_btnApply_clicked();

  void on_btnClose_clicked();

  void on_inLabelFilter_textChanged(const QString &arg);

  void on_btnSelectAll_clicked();

  void on_btnClearAll_clicked();

  void on_sigmaX_textEdited(const QString &newText);
  void on_sigmaY_textEdited(const QString &newText);
  void on_sigmaZ_textEdited(const QString &newText);

private:
  Ui::SmoothLabelsDialog *ui;

  SmartPtr<SmoothLabelsModel> m_Model;

  QStandardItemModel *m_simodel;

  QSortFilterProxyModel *m_LabelListFilterModel;

  virtual void showEvent(QShowEvent *e);

  void setAllLabelCheckStates(Qt::CheckState chkState);

  void syncSigmas(int8_t dim, const QString &newText);
};


#endif // SMOOTHLABELSDIALOG_H
