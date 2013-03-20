#ifndef LABELEDITORDIALOG_H
#define LABELEDITORDIALOG_H

#include <QDialog>

class LabelEditorModel;
class QStandardItemModel;
class QSortFilterProxyModel;

namespace Ui {
    class LabelEditorDialog;
}

class LabelEditorDialog : public QDialog
{
  Q_OBJECT

public:
  explicit LabelEditorDialog(QWidget *parent = 0);
  ~LabelEditorDialog();

  void SetModel(LabelEditorModel *model);

private slots:
  void on_btnClose_clicked();

  void on_btnNew_clicked();

  void on_btnDuplicate_clicked();

  void on_btnDelete_clicked();

  void on_inLabelId_editingFinished();

  void on_actionResetLabels_triggered();

  void on_inLabelFilter_textChanged(const QString &arg1);

private:
  Ui::LabelEditorDialog *ui;

  LabelEditorModel *m_Model;

  // The Qt model tied to the QListView
  QSortFilterProxyModel *m_LabelListFilterModel;
};

#endif // LABELEDITORDIALOG_H
