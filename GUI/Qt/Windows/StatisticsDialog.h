#ifndef STATISTICSDIALOG_H
#define STATISTICSDIALOG_H

#include <QDialog>

namespace Ui {
class StatisticsDialog;
}

class GlobalUIModel;
class QStandardItemModel;
class SegmentationStatistics;

class StatisticsDialog : public QDialog
{
  Q_OBJECT

public:

  void SetModel(GlobalUIModel *model);
  void Activate();
  
public:
  explicit StatisticsDialog(QWidget *parent = 0);
  ~StatisticsDialog();
  
private slots:
  void on_btnUpdate_clicked();

  void on_btnCopy_clicked();

  void on_btnExport_clicked();

private:
  Ui::StatisticsDialog *ui;

  GlobalUIModel *m_Model;
  QStandardItemModel *m_ItemModel;
  SegmentationStatistics *m_Stats;

  void FillTable();
};

#endif // STATISTICSDIALOG_H
