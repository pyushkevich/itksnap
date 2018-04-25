#ifndef DOWNLOADTICKETDIALOG_H
#define DOWNLOADTICKETDIALOG_H

#include <QDialog>

class DistributedSegmentationModel;

namespace Ui {
class DownloadTicketDialog;
}

class DownloadTicketDialog : public QDialog
{
  Q_OBJECT

public:
  explicit DownloadTicketDialog(QWidget *parent = 0);
  ~DownloadTicketDialog();

  void SetModel(DistributedSegmentationModel *model);

  void InitializeToSuggestedFilename();

  static QString showDialog(QWidget *parent, DistributedSegmentationModel *model);



private:
  Ui::DownloadTicketDialog *ui;

  DistributedSegmentationModel *m_Model;
};

#endif // DOWNLOADTICKETDIALOG_H
