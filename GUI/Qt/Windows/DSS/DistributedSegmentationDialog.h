#ifndef DISTRIBUTEDSEGMENTATIONDIALOG_H
#define DISTRIBUTEDSEGMENTATIONDIALOG_H

#include <QDialog>
#include <QTimer>
#include <QModelIndex>
#include <QStyledItemDelegate>

#include <QComboBox>

/** This class only required for legacy Qt4 support */
class TagComboDelegatePopupShow : public QObject
{
  Q_OBJECT
public:
  TagComboDelegatePopupShow(QComboBox *parent) : QObject(parent) {combo = parent;}

public slots:
  void showPopup() { combo->showPopup(); }

private:
  QComboBox *combo;
};

namespace Ui {
class DistributedSegmentationDialog;
}

class DistributedSegmentationModel;
class EventBucket;
class QAbstractItemView;
class QComboBox;
class DownloadTicketDialog;




class TagComboDelegate : public QStyledItemDelegate
{
  Q_OBJECT

public:

  TagComboDelegate(DistributedSegmentationModel *model, QAbstractItemView *parent);

  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;

  void setEditorData(QWidget *editor, const QModelIndex &index) const;

  void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;

  void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:
  DistributedSegmentationModel *m_Model;
};


class AttachmentComboDelegate : public QStyledItemDelegate
{
  Q_OBJECT
public:
  AttachmentComboDelegate(DistributedSegmentationModel *model, QObject *parent = NULL);

  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;

  void setEditorData(QWidget *editor, const QModelIndex &index) const;

  void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;

  void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;

public slots:

  void onMenuAction(QAction *action);

private:
  DistributedSegmentationModel *m_Model;
};


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
  void updateTicketDetail();

  void onTicketListRefreshTimer();
  void onSelectedTicketRefreshTimer();

private slots:
  void on_btnGetToken_clicked();

  void on_btnSubmit_clicked();

  void on_btnDownload_clicked();

  void on_btnDelete_clicked();

  void on_btnManageServers_clicked();

  void on_btnViewServices_clicked();

  void on_btnResetTags_clicked();

  void on_btnOpenDownloaded_clicked();

  void on_btnOpenSource_clicked();

private:
  Ui::DistributedSegmentationDialog *ui;

  DistributedSegmentationModel *m_Model;


  // A timer that updates the ticket listing
  QTimer *m_TicketDetailRefreshTimer;
  QTimer *m_TicketListingRefreshTimer;

  void LaunchTicketListingRefresh();
  void LaunchTicketDetailRefresh();
};

#endif // DISTRIBUTEDSEGMENTATIONDIALOG_H
