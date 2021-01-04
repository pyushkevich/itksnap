#ifndef SMOOTHLABELSDIALOG_H
#define SMOOTHLABELSDIALOG_H

// issue #24: Add label smoothing feature

#include<QDialog>
#include<SNAPCommon.h>

class SmoothLabelsModel;
class QStandardItemModel;
class QSortFilterProxyModel;

namespace Ui {
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

private:
  Ui::SmoothLabelsDialog *ui;

  SmartPtr<SmoothLabelsModel> m_Model;

  QSortFilterProxyModel *m_LabelListFilterModel;

  virtual void showEvent(QShowEvent *e);
};


#endif // SMOOTHLABELSDIALOG_H
