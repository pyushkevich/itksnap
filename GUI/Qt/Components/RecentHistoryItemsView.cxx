#include "RecentHistoryItemsView.h"
#include "ui_RecentHistoryItemsView.h"
#include "HistoryManager.h"
#include <QListView>
#include <QItemDelegate>
#include <QPainter>
#include <QMenu>
#include <QAction>
#include <HistoryQListModel.h>
#include <SNAPQtCommon.h>
#include <IRISApplication.h>
#include <SystemInterface.h>
#include <GlobalUIModel.h>

class HistoryListItemDelegate : public QItemDelegate
{
public:
  HistoryListItemDelegate(QWidget *parent) : QItemDelegate(parent) {}

  void paint(QPainter *painter,
             const QStyleOptionViewItem &option,
             const QModelIndex &index) const
  {
    if(!(option.state & QStyle::State_MouseOver))
      {
      painter->setOpacity(0.8);
      }
    else
      {
      painter->setBackground(QBrush(Qt::black));
      }

    QItemDelegate::paint(painter, option, index);
  }
};

RecentHistoryItemsView::RecentHistoryItemsView(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::RecentHistoryItemsView)
{
  ui->setupUi(this);

  // Delegate for history
  HistoryListItemDelegate *del = new HistoryListItemDelegate(ui->listRecent);
  ui->listRecent->setItemDelegate(del);
  ui->listRecent->setUniformItemSizes(true);

  // Set up the popup for the recent list
  m_RecentListPopup = new QMenu(this);
  m_RecentListPopupAction = m_RecentListPopup->addAction(
        "Remove from recent image list", this,
        SLOT(OnRemoveRecentImageListItem()));
}

RecentHistoryItemsView::~RecentHistoryItemsView()
{
  delete ui;
}

void RecentHistoryItemsView::Initialize(GlobalUIModel *model, std::string history)
{
  m_Model = model;
  m_History = history;

  // Create a model for the table of recent images and connect to the widget
  m_HistoryModel = new HistoryQListModel(ui->listRecent);
  m_HistoryModel->Initialize(model, history);
  ui->listRecent->setModel(m_HistoryModel);
}

void RecentHistoryItemsView::on_listRecent_clicked(const QModelIndex &index)
{
  // Load the appropriate image
  if(index.isValid())
    {
    QString filename = ui->listRecent->model()->data(index, Qt::UserRole).toString();
    emit RecentItemSelected(filename);
    }
}

void RecentHistoryItemsView::on_listRecent_customContextMenuRequested(const QPoint &pos)
{
  QModelIndex idx = ui->listRecent->indexAt(pos);
  if(idx.isValid())
    {
    QVariant data = ui->listRecent->model()->data(idx, Qt::UserRole);
    if(!data.isNull())
      {
      m_RecentListPopupAction->setData(data);
      m_RecentListPopup->popup(QCursor::pos());
      }
    }
}

void RecentHistoryItemsView::OnRemoveRecentImageListItem()
{
  QAction *action = qobject_cast<QAction *>(this->sender());
  QString filename = action->data().toString();
  HistoryManager *hm =
      m_Model->GetDriver()->GetSystemInterface()->GetHistoryManager();
  hm->DeleteHistoryItem(m_History, to_utf8(filename));
}


