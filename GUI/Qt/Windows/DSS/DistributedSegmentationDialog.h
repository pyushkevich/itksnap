#ifndef DISTRIBUTEDSEGMENTATIONDIALOG_H
#define DISTRIBUTEDSEGMENTATIONDIALOG_H

#include <QDialog>
#include <QThread>

namespace Ui {
class DistributedSegmentationDialog;
}

class DistributedSegmentationModel;
class EventBucket;


class DistributedSegmentationDialog : public QDialog
{
  Q_OBJECT

public:
  explicit DistributedSegmentationDialog(QWidget *parent = 0);
  ~DistributedSegmentationDialog();

  void SetModel(DistributedSegmentationModel *model);

public slots:

  virtual void onModelUpdate(const EventBucket &bucket);

  virtual void updateServerStatus();

private slots:
  void on_btnGetToken_clicked();

private:
  Ui::DistributedSegmentationDialog *ui;

  DistributedSegmentationModel *m_Model;

};

#endif // DISTRIBUTEDSEGMENTATIONDIALOG_H
