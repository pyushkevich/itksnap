#include "DistributedSegmentationDialog.h"
#include "ui_DistributedSegmentationDialog.h"
#include "DistributedSegmentationModel.h"
#include "QtComboBoxCoupling.h"
#include "QtLineEditCoupling.h"
#include "QtLabelCoupling.h"
#include <QDesktopServices>
#include <QUrl>
#include <QtConcurrent>
#include <QtTableWidgetCoupling.h>
#include "SNAPQtCommon.h"
#include "Registry.h"
#include "SaveModifiedLayersDialog.h"
#include "QtCursorOverride.h"
#include <QProgressDialog>
#include <QtReporterDelegates.h>
#include <QtAbstractItemViewCoupling.h>
#include <QTimer>


DistributedSegmentationDialog::DistributedSegmentationDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::DistributedSegmentationDialog)
{
  ui->setupUi(this);
  m_Model = NULL;

  // Create the model for the table view
  QStandardItemModel *ticket_list_model = new QStandardItemModel();
  ticket_list_model->setColumnCount(3);
  ticket_list_model->setHorizontalHeaderItem(0, new QStandardItem("Ticket"));
  ticket_list_model->setHorizontalHeaderItem(1, new QStandardItem("Service"));
  ticket_list_model->setHorizontalHeaderItem(2, new QStandardItem("Status"));

  QList<QStandardItem *> dummy;
  dummy.push_back(new QStandardItem("blah"));
  dummy.push_back(new QStandardItem("glooh"));
  dummy.push_back(new QStandardItem("googee"));
  ticket_list_model->appendRow(dummy);

  ui->tblTickets->setModel(ticket_list_model);

  QItemSelectionModel *sel = new QItemSelectionModel(ticket_list_model);
  ui->tblTickets->setSelectionModel(sel);
}

DistributedSegmentationDialog::~DistributedSegmentationDialog()
{
  delete ui;
}

class TagRowDescMapper
{
public:
  static void updateRowDescription(QTableWidget *w, int index, const dss_model::TagTargetSpec &spec)
  {
    // Set the row contents
    w->item(index, 0)->setText(from_utf8(spec.tag_spec.name));
    w->item(index, 0)->setToolTip(from_utf8(spec.tag_spec.hint));
    w->item(index, 1)->setText(from_utf8(dss_model::tag_type_strings[spec.tag_spec.type]));
    w->item(index, 2)->setCheckState(spec.tag_spec.required ? Qt::Checked : Qt::Unchecked);

    w->item(index, 3)->setText(from_utf8(spec.desc));
    if(spec.object_id == 0 && spec.tag_spec.required)
      w->item(index, 3)->setTextColor(QColor(Qt::darkRed));
    else
      w->item(index, 3)->setTextColor(QColor(Qt::black));
    // w->setCellWidget(index, 3, new QPushButton("assign"));

    // Set editable properties
    for(int k = 0; k < 3; k++)
      {
      toggle_flags_off(w->item(index, k), Qt::ItemIsEditable);
      }
  }
};

/**
 * Traits mapping ticket summary to rows in a table
 */
class TicketStatusSummaryRowTraits
{
public:

  static int columnCount() { return 3; }

  static void updateItem(QStandardItem *item, int column,
                         dss_model::TicketId ticket_id,
                         const dss_model::TicketStatusSummary &status)
  {
    // Column-specific
    if(column == 0)
      {
      item->setData(QVariant((qlonglong) status.id), Qt::DisplayRole | Qt::EditRole);
      }
    else if(column == 1)
      {
      item->setText(from_utf8(status.service_name));
      }
    else if(column == 2)
      {
      item->setText(from_utf8(dss_model::ticket_status_strings[status.status]));
      }

    // Flags
    toggle_flags_off(item, Qt::ItemIsEditable);
    item->setData((qlonglong) ticket_id, Qt::UserRole);
  }

  static dss_model::TicketId getItemValue(QStandardItem *item)
  {
    return static_cast<dss_model::TicketId>(item->data(Qt::UserRole).value<qlonglong>());
  }
};


#include <QStyledItemDelegate>

class TagComboDelegate : public QStyledItemDelegate
{
public:
  TagComboDelegate(DistributedSegmentationModel *model, QObject *parent = NULL)
    : QStyledItemDelegate(parent)
  {
    m_Model = model;
  }

  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
  {

    if(index.column() == 3)
      {
      QComboBox *combo = new QComboBox(parent);
      makeCoupling(combo, m_Model->GetCurrentTagImageLayerModel());
      return combo;
      }
    else
      return QStyledItemDelegate::createEditor(parent, option, index);
  }

  void setEditorData(QWidget *editor, const QModelIndex &index) const
  {
    if(index.column() == 3)
      {
      }
    else
      QStyledItemDelegate::setEditorData(editor, index);

  }

  void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
  {
    if(index.column() == 3)
      {
      }
    else
      QStyledItemDelegate::setModelData(editor, model, index);
  }

  void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
  {
    editor->setGeometry(option.rect);
  }

private:
  DistributedSegmentationModel *m_Model;
};

void DistributedSegmentationDialog::SetModel(DistributedSegmentationModel *model)
{
  // Store the model
  m_Model = model;

  // Connect the widgets
  makeCoupling(ui->inServer, m_Model->GetServerURLModel());
  makeCoupling(ui->inToken, m_Model->GetTokenModel());
  makeCoupling(ui->outStatus, m_Model->GetServerStatusStringModel());
  makeCoupling(ui->inService, m_Model->GetCurrentServiceModel());
  makeCoupling(ui->outServiceDesc, m_Model->GetServiceDescriptionModel());

  // Couple the tag list widget
  DefaultTableWidgetRowTraits<int, dss_model::TagTargetSpec, TagRowDescMapper> tag_row_traits;
  makeMultiRowCoupling(ui->tblTags, m_Model->GetTagListModel(), tag_row_traits);

  // Create an item delegate for the fourth column
  ui->tblTags->setItemDelegateForColumn(3, new TagComboDelegate(m_Model, this));

  // Couple the ticket listing widget
  typedef DefaultWidgetValueTraits<dss_model::TicketId, QAbstractItemView> TicketSummaryValueTraits;
  typedef QStandardItemModelWidgetDomainTraits<
      DistributedSegmentationModel::TicketListingDomain,
      TicketStatusSummaryRowTraits> TicketSummaryDomainTraits;

  makeCoupling((QAbstractItemView *) ui->tblTickets, m_Model->GetTicketListModel(),
               TicketSummaryValueTraits(), TicketSummaryDomainTraits());


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


  // Activations
  activateOnFlag(ui->btnSubmit, m_Model, DistributedSegmentationModel::UIF_TAGS_ASSIGNED);
  activateOnFlag(ui->tabSubmit, m_Model, DistributedSegmentationModel::UIF_AUTHENTICATED);
  activateOnFlag(ui->tabResults, m_Model, DistributedSegmentationModel::UIF_AUTHENTICATED);

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
    // We need to refresh the ticket listing
    LaunchTicketListingRefresh();
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

  // Schedule another listing update
  QTimer::singleShot(4000, this, SLOT(onTicketListRefreshTimer()));

}

void DistributedSegmentationDialog::onTicketListRefreshTimer()
{
  // If there is no model or the dialog is hidden, there is nothing to do
  if(!m_Model || !this->isVisible())
    return;

  // Run the refresh
  LaunchTicketListingRefresh();
}

void DistributedSegmentationDialog::on_btnGetToken_clicked()
{
  // Open the web browsersdf
  QDesktopServices::openUrl(QUrl(m_Model->GetURL("token").c_str()));
}

void DistributedSegmentationDialog::on_btnSubmit_clicked()
{
  // Apply the tags to the workspace
  m_Model->ApplyTagsToTargets();

  // Save the workspace
  if(!SaveModifiedLayersDialog::PromptForUnsavedChanges(m_Model->GetParent()))
    return;

  // Show a progress dialog
  QProgressDialog *progress = new QProgressDialog();
  QtProgressReporterDelegate progress_delegate;
  progress_delegate.SetProgressDialog(progress);

  progress->setLabelText  ("Uploading workspace...");
  progress->setMinimumDuration(0);

  // Submit the workspace
  m_Model->SubmitWorkspace(&progress_delegate);

  progress->hide();
  delete progress;
}
