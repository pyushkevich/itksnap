#include "DistributedSegmentationDialog.h"
#include "ui_DistributedSegmentationDialog.h"
#include "DistributedSegmentationModel.h"
#include "QtComboBoxCoupling.h"
#include "QtLineEditCoupling.h"
#include "QtLabelCoupling.h"
#include "QtPagedWidgetCoupling.h"
#include <QDesktopServices>
#include <QUrl>

#if QT_VERSION >= 0x050000
#include <QtConcurrent>
#else
#include <QtCore>
#endif

#include "SNAPQtCommon.h"
#include "Registry.h"
#include "SaveModifiedLayersDialog.h"
#include "QtCursorOverride.h"
#include <QProgressDialog>
#include <QtReporterDelegates.h>
#include <QtAbstractItemViewCoupling.h>
#include <QtProgressBarCoupling.h>
#include <QTimer>
#include "IRISException.h"
#include "MainImageWindow.h"
#include <QToolButton>
#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>

#include "DownloadTicketDialog.h"

/**
 * Traits mapping tags to row in a table
 */
class TagListRowTraits
{
public:
  static int columnCount() { return 4; }

  static void updateRow(QList<QStandardItem *> items,
                        int tag_id,
                        const dss_model::TagTargetSpec &spec)
  {
    // Set the row contents
    items[0]->setText(from_utf8(spec.tag_spec.name));
    items[0]->setToolTip(from_utf8(spec.tag_spec.hint));
    items[1]->setText(from_utf8(dss_model::tag_type_strings[spec.tag_spec.type]));
    items[2]->setCheckState(spec.tag_spec.required ? Qt::Checked : Qt::Unchecked);
    items[2]->setTextAlignment(Qt::AlignHCenter);

    items[3]->setText(from_utf8(spec.desc));
    if(spec.object_id == 0 && spec.tag_spec.required)
      items[3]->setForeground(QColor(Qt::darkRed));
    else
      items[3]->setForeground(QBrush());

    // Set editable properties
    for(int k = 0; k < 3; k++)
      toggle_flags_off(items[k], Qt::ItemIsEditable);

    // Set the data
    items[0]->setData(tag_id, Qt::UserRole);
  }

  static int getRowValue(QList<QStandardItem *> items)
  {
    return items[0]->data(Qt::UserRole).value<int>();
  }


};

/**
 * Traits mapping ticket summary to rows in a table
 */
class TicketStatusSummaryRowTraits
{
public:

  static int columnCount() { return 3; }

  static void updateRow(QList<QStandardItem *> items,
                        dss_model::IdType ticket_id,
                        const dss_model::TicketStatusSummary &status)
  {
    // Set data for the items
    items[0]->setData(QVariant((qlonglong) status.id), Qt::DisplayRole | Qt::EditRole);
    items[1]->setText(from_utf8(status.service_name));
    items[2]->setText(from_utf8(dss_model::ticket_status_strings[status.status]));

    // Set flags on all items
    foreach (QStandardItem *item, items)
      toggle_flags_off(item, Qt::ItemIsEditable);

    // Set the data for the first item
    items[0]->setData((qlonglong) ticket_id, Qt::UserRole);
  }

  static dss_model::IdType getRowValue(QList<QStandardItem *> items)
  {
    return static_cast<dss_model::IdType>(items[0]->data(Qt::UserRole).value<qlonglong>());
  }
};




/**
 * Traits mapping ticket log entries to rows in a table
 */
class TicketLogEntryRowTraits
{
public:

  static int columnCount() { return 3; }

  static void updateRow(QList<QStandardItem *> items,
                        dss_model::IdType log_id,
                        const dss_model::TicketLogEntry &entry)
  {
    // Icons for the message type
    static QStringList icons = (QStringList()
                                << ":/root/icons8_info_16.png"
                                << ":/root/icons8_error_16.png"
                                << ":/root/icons8_no_entry_16.png" << "");

    // Compute date in easy to read format
    QString t_stamp = from_utf8(entry.atime).split(".").first();
    QDateTime dt = QDateTime::fromString(t_stamp, "yyyy-MM-dd hh:mm:ss");
    dt.setTimeSpec(Qt::UTC);

    // First item is the date/time
    items[0]->setText(get_user_friendly_date_string(dt));
    items[0]->setIcon(QIcon(icons[entry.type]));
    toggle_flags_off(items[0], Qt::ItemIsEditable);

    // Second time is the message
    items[1]->setText(from_utf8(entry.text));
    toggle_flags_off(items[1], Qt::ItemIsEditable);

    // Last item is the attachment menu
    if(entry.attachments.size() > 0)
      {
      items[2]->setIcon(QIcon(":/root/icons8_attach_16.png"));
      items[2]->setData((int) entry.attachments.size(), Qt::DisplayRole);
      }
    else
      {
      items[2]->setIcon(QIcon());
      items[2]->setData(QVariant(), Qt::DisplayRole);
      toggle_flags_off(items[2], Qt::ItemIsEditable);
      }

    // Flags
    items[0]->setData((qlonglong) log_id, Qt::UserRole);
  }

  static dss_model::IdType getRowValue(QList<QStandardItem *> items)
  {
    return static_cast<dss_model::IdType>(items[0]->data(Qt::UserRole).value<qlonglong>());
  }
};


/**
 * Traits for mapping status codes to a label
 */
template<>
class DefaultWidgetValueTraits<dss_model::AuthResponse, QLabel>
    : public WidgetValueTraitsBase<dss_model::AuthResponse, QLabel *>
{
public:
  typedef dss_model::AuthResponse TAtomic;

  virtual TAtomic GetValue(QLabel *w)
  {
    return dss_model::AuthResponse();
  }

  virtual void SetValue(QLabel *w, const TAtomic &value)
  {
    switch(value.status)
      {
      case dss_model::AUTH_NOT_CONNECTED:
        w->setText("Not Connected");
        w->setStyleSheet("color: darkred; font-weight: bold;");
        break;
      case dss_model::AUTH_CONNECTED_NOT_AUTHENTICATED:
        w->setText("Connected but Not Logged In");
        w->setStyleSheet("color: darkred; font-weight: bold;");
        break;
      case dss_model::AUTH_AUTHENTICATED:
        w->setText(QString("Logged in as %1").arg(from_utf8(value.user_email)));
        w->setStyleSheet("color: darkgreen; font-weight: bold;");
        break;
      }
  }
};

/**
 * Traits for displaying the ticket number
 */
class SelectedTicketHeadingTraits
    : public WidgetValueTraitsBase<dss_model::IdType, QLabel *>
{
public:
  virtual dss_model::IdType GetValue(QLabel *w) { return 0; }

  virtual void SetValue(QLabel *w, const dss_model::IdType &value)
  {
    if(value > 0)
      w->setText(QString("Ticket %1").arg(value));
    else
      w->setText("Selected Ticket");
  }
};

/*

template <>
class DefaultWidgetDomainTraits<DistributedSegmentationModel::ServerStatusDomain, QLabel>
     : public WidgetDomainTraitsBase<DistributedSegmentationModel::ServerStatusDomain, QLabel *>
{
public:
  typedef DistributedSegmentationModel::ServerStatusDomain TDomain;

  virtual void SetDomain(QLabel *w, const TDomain &domain) ITK_OVERRIDE {}
  virtual TDomain GetDomain(QLabel * w) ITK_OVERRIDE { return TDomain(); }
};

*/








DistributedSegmentationDialog::DistributedSegmentationDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::DistributedSegmentationDialog)
{
  ui->setupUi(this);
  m_Model = NULL;

  // Create the model for the tag listing
  QStandardItemModel *tags_model = new QStandardItemModel();
  tags_model->setHorizontalHeaderLabels(
        QStringList() << "Tag" << "Type" << "Required" << "Target Object");
  ui->tblTags->setModel(tags_model);

  // Set the sizing of the columns
#if QT_VERSION >= 0x050000
  ui->tblTags->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  ui->tblTags->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  ui->tblTags->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  ui->tblTags->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
#else
  ui->tblTags->horizontalHeader()->setResizeMode(0, QHeaderView::ResizeToContents);
  ui->tblTags->horizontalHeader()->setResizeMode(1, QHeaderView::ResizeToContents);
  ui->tblTags->horizontalHeader()->setResizeMode(2, QHeaderView::ResizeToContents);
  ui->tblTags->horizontalHeader()->setResizeMode(3, QHeaderView::Stretch);
#endif

  // Create the model for the table view
  QStandardItemModel *ticket_list_model = new QStandardItemModel();
  ticket_list_model->setHorizontalHeaderLabels(QStringList() << "Ticket" << "Service" << "Status");
  ui->tblTickets->setModel(ticket_list_model);

  QItemSelectionModel *sel = new QItemSelectionModel(ticket_list_model);
  ui->tblTickets->setSelectionModel(sel);

#if QT_VERSION >= 0x050000
  ui->tblTickets->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  ui->tblTickets->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
  ui->tblTickets->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
#else
  ui->tblTickets->horizontalHeader()->setResizeMode(0, QHeaderView::ResizeToContents);
  ui->tblTickets->horizontalHeader()->setResizeMode(1, QHeaderView::Stretch);
  ui->tblTickets->horizontalHeader()->setResizeMode(2, QHeaderView::ResizeToContents);
#endif

  // Create model for the logs
  QStandardItemModel *log_entry_list_model = new QStandardItemModel();
  log_entry_list_model->setHorizontalHeaderLabels(QStringList() << "Time" << "Message" << "Att");
  ui->tblLog->setModel(log_entry_list_model);

#if QT_VERSION >= 0x050000
  ui->tblLog->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  ui->tblLog->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
  ui->tblLog->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
#else
  ui->tblLog->horizontalHeader()->setResizeMode(0, QHeaderView::ResizeToContents);
  ui->tblLog->horizontalHeader()->setResizeMode(1, QHeaderView::Stretch);
  ui->tblLog->horizontalHeader()->setResizeMode(2, QHeaderView::ResizeToContents);
#endif

  // The ticket detail timer should fire at regular intervals
  m_TicketDetailRefreshTimer = new QTimer(this);
  connect(m_TicketDetailRefreshTimer, SIGNAL(timeout()), this, SLOT(onSelectedTicketRefreshTimer()));
  m_TicketDetailRefreshTimer->start(4000);

  // The ticket listing timer should also fire at regular intervalse
  m_TicketListingRefreshTimer = new QTimer(this);
  connect(m_TicketListingRefreshTimer, SIGNAL(timeout()), this, SLOT(onTicketListRefreshTimer()));
  m_TicketListingRefreshTimer->start(4000);
}

DistributedSegmentationDialog::~DistributedSegmentationDialog()
{
  delete ui;
}


void DistributedSegmentationDialog::SetModel(DistributedSegmentationModel *model)
{
  // Store the model
  m_Model = model;

  // Connect the widgets
  makeCoupling(ui->inServer, m_Model->GetServerURLModel());
  makeCoupling(ui->inToken, m_Model->GetTokenModel());
  makeCoupling(ui->outStatus, m_Model->GetServerStatusModel());
  makeCoupling(ui->inService, m_Model->GetCurrentServiceModel());
  makeCoupling(ui->outServiceDesc, m_Model->GetServiceDescriptionModel());
  makeCoupling(ui->outProgress, m_Model->GetSelectedTicketProgressModel());
  makeCoupling(ui->outQueuePos, m_Model->GetSelectedTicketQueuePositionModel());

  // Coupling for the progress/queue stack page
  std::map<dss_model::TicketStatus, QWidget *> status_to_page_map;
  for(int i = 0; i < dss_model::STATUS_UNKNOWN; i++)
    status_to_page_map[(dss_model::TicketStatus) i] = (i == dss_model::STATUS_READY) ? ui->pgQueuePos : ui->pgProgress;
  makePagedWidgetCoupling(ui->stackProgress, m_Model->GetSelectedTicketStatusModel(), status_to_page_map);

  // Couple the tag list widget
  makeMultiRowCoupling((QAbstractItemView *) ui->tblTags,
                       m_Model->GetTagListModel(),
                       TagListRowTraits());

  // Create an item delegate for the fourth column
  ui->tblTags->setItemDelegateForColumn(3, new TagComboDelegate(m_Model, ui->tblTags));

  // Couple the ticket listing widget
  makeMultiRowCoupling((QAbstractItemView *) ui->tblTickets, m_Model->GetTicketListModel(),
                       TicketStatusSummaryRowTraits());

  // Couple the log listing widget
  makeMultiRowCoupling((QAbstractItemView *) ui->tblLog,
                       m_Model->GetSelectedTicketLogModel(),
                       TicketLogEntryRowTraits());

  // Ticket number
  makeDomainlessCoupling(ui->outTicketId, m_Model->GetTicketListModel());

  // Local workspace
  makeCoupling(ui->outTicketWorkspace, m_Model->GetSelectedTicketLocalWorkspaceModel());
  makeCoupling(ui->outTicketDownloadLocation, m_Model->GetSelectedTicketResultWorkspaceModel());

  // Create a delegate for the attachment column
  ui->tblLog->setItemDelegateForColumn(2, new AttachmentComboDelegate(m_Model, this));


  // Listen for server changes
  LatentITKEventNotifier::connect(
        model, DistributedSegmentationModel::ServerChangeEvent(),
        this, SLOT(onModelUpdate(const EventBucket &)));

  // Listen for server changes
  LatentITKEventNotifier::connect(
        model, DistributedSegmentationModel::ServiceChangeEvent(),
        this, SLOT(onModelUpdate(const EventBucket &)));

  // Listen for changes in connectivity status
  LatentITKEventNotifier::connect(
        model->GetServerStatusModel(), ValueChangedEvent(),
        this, SLOT(onModelUpdate(const EventBucket &)));

  // Listen for changes in selected ticket
  LatentITKEventNotifier::connect(
        model->GetTicketListModel(), ValueChangedEvent(),
        this, SLOT(onModelUpdate(const EventBucket &)));


  // Activations
  activateOnFlag(ui->btnSubmit, m_Model, DistributedSegmentationModel::UIF_TAGS_ASSIGNED);
  activateOnFlag(ui->tabSubmit, m_Model, DistributedSegmentationModel::UIF_AUTHENTICATED);
  activateOnFlag(ui->tabResults, m_Model, DistributedSegmentationModel::UIF_AUTHENTICATED);
  activateOnFlag(ui->btnDownload, m_Model, DistributedSegmentationModel::UIF_CAN_DOWNLOAD);
  activateOnFlag(ui->btnOpenSource, m_Model, DistributedSegmentationModel::UIF_TICKET_HAS_LOCAL_SOURCE);
  activateOnFlag(ui->btnOpenDownloaded, m_Model, DistributedSegmentationModel::UIF_TICKET_HAS_LOCAL_RESULT);

  // Get the model to fire off a server change event - to cause a login
  m_Model->InvokeEvent(DistributedSegmentationModel::ServerChangeEvent());

}

void DistributedSegmentationDialog::LaunchTicketListingRefresh()
{
  QFuture<dss_model::TicketListingResponse> future =
      QtConcurrent::run(DistributedSegmentationModel::AsyncGetTicketListing);

  QFutureWatcher<dss_model::TicketListingResponse> *watcher =
      new QFutureWatcher<dss_model::TicketListingResponse>();

  connect(watcher, SIGNAL(finished()), this, SLOT(updateTicketListing()));
  watcher->setFuture(future);
}

void DistributedSegmentationDialog::LaunchTicketDetailRefresh()
{
  dss_model::IdType selected_ticket_id;
  if(m_Model->GetTicketListModel()->GetValueAndDomain(selected_ticket_id, NULL)
     && selected_ticket_id >= 0)
    {
    QFuture<dss_model::TicketDetailResponse> future =
        QtConcurrent::run(DistributedSegmentationModel::AsyncGetTicketDetails,
                          selected_ticket_id, m_Model->GetLastLogIdOfSelectedTicket());

    QFutureWatcher<dss_model::TicketDetailResponse> *watcher =
        new QFutureWatcher<dss_model::TicketDetailResponse>();
    connect(watcher, SIGNAL(finished()), this, SLOT(updateTicketDetail()));
    watcher->setFuture(future);
    }
}

void DistributedSegmentationDialog::onModelUpdate(const EventBucket &bucket)
{
  if(bucket.HasEvent(DistributedSegmentationModel::ServerChangeEvent()))
    {
    // The server has changed. We should launch a separate job to connect to the
    // server, get the list of services, and update the status.
    QFuture<dss_model::StatusCheckResponse> future =
        QtConcurrent::run(DistributedSegmentationModel::AsyncCheckStatus,
                          m_Model->GetURL(""), m_Model->GetToken());

    QFutureWatcher<dss_model::StatusCheckResponse> *watcher =
        new QFutureWatcher<dss_model::StatusCheckResponse>();
    connect(watcher, SIGNAL(finished()), this, SLOT(updateServerStatus()));
    watcher->setFuture(future);    
    }
  if(bucket.HasEvent(DistributedSegmentationModel::ServiceChangeEvent()))
    {
    // The service has changed. Launch a separate job to get the service details
    QFuture<dss_model::ServiceDetailResponse> future =
        QtConcurrent::run(DistributedSegmentationModel::AsyncGetServiceDetails,
                          m_Model->GetCurrentServiceGitHash());

    QFutureWatcher<dss_model::ServiceDetailResponse> *watcher =
        new QFutureWatcher<dss_model::ServiceDetailResponse>();
    connect(watcher, SIGNAL(finished()), this, SLOT(updateServiceDetail()));
    watcher->setFuture(future);
    }
  if(bucket.HasEvent(ValueChangedEvent(), m_Model->GetServerStatusModel()))
    {
    // Restart the refresh timer - so that the next time is fires is delayed
    m_TicketListingRefreshTimer->start(m_TicketListingRefreshTimer->interval());

    // We need to refresh the ticket listing
    LaunchTicketListingRefresh();
    }
  if(bucket.HasEvent(ValueChangedEvent(), m_Model->GetTicketListModel()))
    {
    // Restart the refresh timer - so that the next time is fires is delayed
    m_TicketDetailRefreshTimer->start(m_TicketDetailRefreshTimer->interval());

    // The ticket has changed. Launch a separate job to get the ticket details
    LaunchTicketDetailRefresh();
    }
}

void DistributedSegmentationDialog::updateServerStatus()
{
  QFutureWatcher<dss_model::StatusCheckResponse> *watcher =
      dynamic_cast<QFutureWatcher<dss_model::StatusCheckResponse> *>(this->sender());

  m_Model->ApplyStatusCheckResponse(watcher->result());

  delete watcher;

}

void DistributedSegmentationDialog::updateServiceDetail()
{
  QFutureWatcher<dss_model::ServiceDetailResponse> *watcher =
      dynamic_cast<QFutureWatcher<dss_model::ServiceDetailResponse> *>(this->sender());

  m_Model->ApplyServiceDetailResponse(watcher->result());

  delete watcher;
}

void DistributedSegmentationDialog::updateTicketListing()
{
  QFutureWatcher<dss_model::TicketListingResponse> *watcher =
      dynamic_cast<QFutureWatcher<dss_model::TicketListingResponse> *>(this->sender());

  m_Model->ApplyTicketListingResponse(watcher->result());

  delete watcher;
}

void DistributedSegmentationDialog::updateTicketDetail()
{
  QFutureWatcher<dss_model::TicketDetailResponse> *watcher =
      dynamic_cast<QFutureWatcher<dss_model::TicketDetailResponse> *>(this->sender());

  m_Model->ApplyTicketDetailResponse(watcher->result());

  delete watcher;
}

void DistributedSegmentationDialog::onTicketListRefreshTimer()
{
  // If there is no model or the dialog is hidden, there is nothing to do
  if(m_Model && this->isVisible())
    {
    // Run the refresh
    LaunchTicketListingRefresh();
    }
}

void DistributedSegmentationDialog::onSelectedTicketRefreshTimer()
{
  // If there is no model or the dialog is hidden, there is nothing to do
  if(m_Model && this->isVisible() && m_Model->GetTicketListModel()->isValid())
    {
    LaunchTicketDetailRefresh();
    }
}

void DistributedSegmentationDialog::on_btnGetToken_clicked()
{
  // Open the web browsersdf
  QDesktopServices::openUrl(QUrl(m_Model->GetURL("token").c_str()));
}

// TODO: this is to support Qt4 scoped pointer
#if QT_VERSION >= 0x050000
typedef QScopedPointer<QProgressDialog, QScopedPointerDeleteLater> QtProgressDialogScopedPointer;
#else
typedef QScopedPointer<QProgressDialog> QtProgressDialogScopedPointer;
#endif

void DistributedSegmentationDialog::on_btnSubmit_clicked()
{
  // Apply the tags to the workspace
  m_Model->ApplyTagsToTargets();

  // If there is no workspace, it has to be created
  MainImageWindow *parent = findParentWidget<MainImageWindow>(this);
  if(!parent->SaveWorkspace(false))
    return;

  // Show a progress dialog
  QtProgressDialogScopedPointer progress(new QProgressDialog(this));
  QtProgressReporterDelegate progress_delegate;
  progress_delegate.SetProgressDialog(progress.data());
  progress->setLabelText("Uploading workspace...");
  progress->setMinimumDuration(0);
  progress->show();
  progress->activateWindow();
  progress->raise();

  // Create a wait cursor
  QtCursorOverride cursy;

  // Process events so that the dialog is actually shown
  QCoreApplication::processEvents();

  // Submit the workspace
  try
    {
    // Do the submit task
    m_Model->SubmitWorkspace(&progress_delegate);

    // Flip over to the results page
    ui->tabWidget->setCurrentWidget(ui->tabResults);
    }
  catch(std::exception &exc)
    {
    ReportNonLethalException(this, exc, "Failed to submit workspace");
    }
}

void DistributedSegmentationDialog::on_btnDownload_clicked()
{
  // Show the dialog to determine the save location
  QString dl_filename = DownloadTicketDialog::showDialog(this, m_Model);

  // No filename - means user canceled
  if(dl_filename.size() == 0)
    return;

  // Show a progress dialog
  QtProgressDialogScopedPointer progress(new QProgressDialog(this));
  QtProgressReporterDelegate progress_delegate;
  progress_delegate.SetProgressDialog(progress.data());
  progress->setLabelText("Downloading workspace...");
  progress->setMinimumDuration(0);
  progress->show();
  progress->activateWindow();
  progress->raise();

  // Create a wait cursor
  QtCursorOverride cursy;

  // Process events so that the dialog is actually shown
  QCoreApplication::processEvents();

  // Download the workspace
  try
  {
    // Download the workspace
    QString ws_file = from_utf8(m_Model->DownloadWorkspace(to_utf8(dl_filename), &progress_delegate));

    // Use main window to open the workspace
    MainImageWindow *parent = findParentWidget<MainImageWindow>(this);

    switch(m_Model->GetDownloadAction())
      {
      case DistributedSegmentationModel::DL_OPEN_CURRENT_WINDOW:
        if(SaveModifiedLayersDialog::PromptForUnsavedChanges(m_Model->GetParent()))
          parent->LoadProject(ws_file);
        break;
      case DistributedSegmentationModel::DL_OPEN_NEW_WINDOW:
        parent->LoadProjectInNewInstance(ws_file);
        break;
      case DistributedSegmentationModel::DL_DONT_OPEN:
        break;
      }
  }
  catch(IRISException &exc)
  {
    ReportNonLethalException(this, exc, "Failed to download workspace");
  }
}


TagComboDelegate::TagComboDelegate(
    DistributedSegmentationModel *model,
    QAbstractItemView *view)
  : QStyledItemDelegate(view)
{
  m_Model = model;
}



QWidget *TagComboDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  // Select the current index in the model
  // QStandardItemModel *sim = dynamic_cast<QStandardItemModel *>(m_View->model());
  // int tag = sim->item(index.row(), 0)->data(Qt::UserRole).value<int>();
  // m_Model->GetTagListModel()->SetValue(tag);
  // m_Model->Update();

  // Make sure that the current index is the actually selected index - otherwise all hell can break loose
  m_Model->GetTagListModel()->SetValue(index.row());

  // Create a combo
  if(m_Model->GetCurrentTagWorkspaceObjectModel()->isValid())
    {
    // We create a customized combo box for this editor with a special "unassigned" field
    QComboBox *combo = new QComboBox(parent);

    ComboBoxWithActionsValueTraits<unsigned long> fancy_value_traits;
    ItemSetComboBoxWithActionsDomainTraits<
        DistributedSegmentationModel::LayerSelectionDomain,
        DefaultComboBoxRowTraits<unsigned long, std::string> > fancy_domain_traits;

    DistributedSegmentationModel::LoadAction load_type = m_Model->GetTagLoadAction(index.row());
    if(load_type == DistributedSegmentationModel::LOAD_MAIN)
      {
      fancy_domain_traits.AddAction(FindUpstreamAction(parent, "actionOpenMain"), 0);
      }
    else if(load_type == DistributedSegmentationModel::LOAD_OVERLAY)
      {
      fancy_domain_traits.AddAction(FindUpstreamAction(parent, "actionAdd_Overlay"), 0);
      }

    makeCoupling(combo, m_Model->GetCurrentTagWorkspaceObjectModel(), fancy_value_traits, fancy_domain_traits);

#if QT_VERSION >= 0x050000
    QTimer::singleShot(0, combo, &QComboBox::showPopup);
#else
    TagComboDelegatePopupShow *popup_show_helper = new TagComboDelegatePopupShow(combo);
    QTimer::singleShot(0, popup_show_helper, SLOT(showPopup()));
#endif

    return combo;
    }

  return QStyledItemDelegate::createEditor(parent, option, index);
}

void TagComboDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
}

void TagComboDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
}

void TagComboDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  editor->setGeometry(option.rect);
}

AttachmentComboDelegate::AttachmentComboDelegate(DistributedSegmentationModel *model, QObject *parent)
  : QStyledItemDelegate(parent)
{
  m_Model = model;
}

QWidget *AttachmentComboDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  // Get details on the selected ticket
  const dss_model::TicketDetailResponse *detail = m_Model->GetSelectedTicketDetail();

  // Get the attachments for that ticket
  if(detail && detail->log.size() > index.row() && detail->log[index.row()].attachments.size())
    {
    // Get the attachments for the log entry in question
    const std::vector<dss_model::Attachment> &att = detail->log[index.row()].attachments;

    // Create a button
    QToolButton *button = new QToolButton(parent);
    button->setIcon(QIcon(":/root/icons8_attach_16.png"));
    button->setPopupMode(QToolButton::InstantPopup);

    // Create a menu for the button
    QMenu *menu = new QMenu(parent);
    connect(menu, SIGNAL(triggered(QAction*)), this, SLOT(onMenuAction(QAction*)));
    for(int i = 0; i < att.size(); i++)
      {
      QAction *action = menu->addAction(QIcon(":/root/icons8_attach_16.png"), from_utf8(att[i].desc));
      action->setData(from_utf8(att[i].url));
      action->setToolTip(QString("<img src='%1'>").arg(from_utf8(att[i].url)));
      }

    button->setMenu(menu);

    QTimer::singleShot(0, button, SLOT(showMenu()));

    return button;
    }
  else
    return NULL;
}

void AttachmentComboDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
}

void AttachmentComboDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
}

void AttachmentComboDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  editor->setGeometry(option.rect);
}

void AttachmentComboDelegate::onMenuAction(QAction *action)
{
  if(action)
    {
    QString url = action->data().toString();
    QDesktopServices::openUrl(QUrl(url));
    }
}

void DistributedSegmentationDialog::on_btnDelete_clicked()
{
  // Delete the current ticket
  try
  {
    m_Model->DeleteSelectedTicket();
  }
  catch(IRISException &exc)
  {
    ReportNonLethalException(this, exc, "Failed to delete selected ticket");
  }


}


void DistributedSegmentationDialog::on_btnManageServers_clicked()
{
  // Get the current list of user URLs
  std::vector<std::string> input_urls = m_Model->GetUserServerList();

#if QT_VERSION >= 0x050000

  // Concatenate them into a multi-line string
  QString input;
  for(int i = 0; i < input_urls.size(); i++)
    input.append(QString("%1%2").arg(i > 0 ? "\n" : "", from_utf8(input_urls[i])));

  // Create a dialog box with a list of servers
  bool ok = false;
  QString servers = QInputDialog::getMultiLineText(
                      this, "Edit Server List",
                      "Enter additional server URLs on separate lines below:", input, &ok);


  if(!ok)
    return;

  // Split into individual strings
  QStringList url_list = servers.split("\n");

#else

  // Concatenate them into a multi-line string
  QString input;
  for(int i = 0; i < input_urls.size(); i++)
    input.append(QString("%1%2").arg(i > 0 ? ";" : "", from_utf8(input_urls[i])));

  // Create a dialog box with a list of servers
  bool ok = false;
  QString servers = QInputDialog::getText(
                      this, "Edit Server List",
                      "Enter additional server URLs separated by semicolon (;):", QLineEdit::Normal, input, &ok);


  if(!ok)
    return;

  // Split into individual strings
  QStringList url_list = servers.split(";");

#endif

  std::vector<std::string> valid_urls;
  foreach(QString url_string, url_list)
    {
    QUrl url(url_string);
    if(!url.isValid() || url.isRelative() || url.isLocalFile() || url.isEmpty())
      {
      QMessageBox::warning(this, "Invalid server URL",
                           QString("%1 is not a valid URL.").arg(url_string));
      }
    else
      {
      valid_urls.push_back(to_utf8(url.toString()));
      }
    }

  // Set the custom URL list
  m_Model->SetUserServerList(valid_urls);
}

void DistributedSegmentationDialog::on_btnViewServices_clicked()
{
  // Open the web browsersdf
  QDesktopServices::openUrl(QUrl(m_Model->GetURL("services").c_str()));
}

void DistributedSegmentationDialog::on_btnResetTags_clicked()
{
  m_Model->ResetTagAssignment();
}

void DistributedSegmentationDialog::on_btnOpenDownloaded_clicked()
{
  // Use main window to open the workspace
  MainImageWindow *parent = findParentWidget<MainImageWindow>(this);

  if(SaveModifiedLayersDialog::PromptForUnsavedChanges(m_Model->GetParent()))
    parent->LoadProject(ui->outTicketDownloadLocation->text());
}

void DistributedSegmentationDialog::on_btnOpenSource_clicked()
{
  // Use main window to open the workspace
  MainImageWindow *parent = findParentWidget<MainImageWindow>(this);

  if(SaveModifiedLayersDialog::PromptForUnsavedChanges(m_Model->GetParent()))
    parent->LoadProject(ui->outTicketWorkspace->text());
}
