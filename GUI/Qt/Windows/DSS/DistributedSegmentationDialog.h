#ifndef DISTRIBUTEDSEGMENTATIONDIALOG_H
#define DISTRIBUTEDSEGMENTATIONDIALOG_H

#include <QDialog>
#include <QTimer>

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

  void updateServerStatus();
  void updateServiceDetail();
  void updateTicketListing();

  void onTicketListRefreshTimer();

private slots:
  void on_btnGetToken_clicked();

  void on_btnSubmit_clicked();

private:
  Ui::DistributedSegmentationDialog *ui;

  DistributedSegmentationModel *m_Model;


  // A timer that updates the ticket listing
  QTimer m_TicketRefreshTimer;

  void LaunchTicketListingRefresh();
};

#endif // DISTRIBUTEDSEGMENTATIONDIALOG_H
