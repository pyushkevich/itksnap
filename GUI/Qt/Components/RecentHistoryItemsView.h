#ifndef RECENTHISTORYITEMSVIEW_H
#define RECENTHISTORYITEMSVIEW_H

#include <QWidget>

class GlobalUIModel;
class QMenu;
class QAction;
class HistoryQListModel;
class QModelIndex;

namespace Ui {
class RecentHistoryItemsView;
}

class RecentHistoryItemsView : public QWidget
{
  Q_OBJECT
  
public:

  /** Set the model and the history name managed by this widget */
  void Initialize(GlobalUIModel *model, std::string history);

  explicit RecentHistoryItemsView(QWidget *parent = 0);
  ~RecentHistoryItemsView();

signals:

  void RecentItemSelected(QString filename);

private slots:

  void OnRemoveRecentImageListItem();

  void on_listRecent_clicked(const QModelIndex &index);

  void on_listRecent_customContextMenuRequested(const QPoint &pos);
  
private:
  Ui::RecentHistoryItemsView *ui;

  GlobalUIModel *m_Model;
  QMenu *m_RecentListPopup;
  QAction *m_RecentListPopupAction;
  HistoryQListModel *m_HistoryModel;
  std::string m_History;

};

#endif // RECENTHISTORYITEMSVIEW_H
