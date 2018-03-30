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


DistributedSegmentationDialog::DistributedSegmentationDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::DistributedSegmentationDialog)
{
  ui->setupUi(this);
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
    // Type of tag
    QString typestr;
    switch(spec.tag_spec.type)
      {
      case dss_model::TAG_LAYER_ANATOMICAL:
        typestr = "Image Layer";
        break;
      case dss_model::TAG_LAYER_MAIN:
        typestr = "Main Image";
        break;
      case dss_model::TAG_SEGMENTATION_LABEL:
        typestr = "Segmentation Label";
        break;
      case dss_model::TAG_POINT_LANDMARK:
        typestr = "Point Landmark";
        break;
      case dss_model::TAG_UNKNOWN:
        typestr = "Unknown";
        break;
      }

    // Set the row contents
    w->item(index, 0)->setText(from_utf8(spec.tag_spec.name));
    w->item(index, 0)->setToolTip(from_utf8(spec.tag_spec.hint));
    w->item(index, 1)->setText(typestr);
    w->item(index, 2)->setCheckState(spec.tag_spec.required ? Qt::Checked : Qt::Unchecked);
    w->item(index, 3)->setText(from_utf8(spec.desc));
    // w->setCellWidget(index, 3, new QPushButton("assign"));

    // Set editable properties
    for(int k = 0; k < 3; k++)
      {
      w->item(index, k)->setFlags(w->item(index, k)->flags() & ~Qt::ItemIsEditable);
      }
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

  DefaultTableWidgetRowTraits<int, dss_model::TagTargetSpec, TagRowDescMapper> tag_row_traits;
  makeMultiRowCoupling(ui->tblTags, m_Model->GetTagListModel(), tag_row_traits);

  // Create an item delegate for the fourth column
  ui->tblTags->setItemDelegateForColumn(3, new TagComboDelegate(m_Model, this));

  // Listen for server changes
  LatentITKEventNotifier::connect(
        model, DistributedSegmentationModel::ServerChangeEvent(),
        this, SLOT(onModelUpdate(const EventBucket &)));

  // Listen for server changes
  LatentITKEventNotifier::connect(
        model, DistributedSegmentationModel::ServiceChangeEvent(),
        this, SLOT(onModelUpdate(const EventBucket &)));

  // Activations
  activateOnFlag(ui->btnSubmit, m_Model, DistributedSegmentationModel::UIF_TAGS_ASSIGNED);

  // Get the model to fire off a server change event - to cause a login
  m_Model->InvokeEvent(DistributedSegmentationModel::ServerChangeEvent());

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

  // Submit the workspace
  m_Model->SubmitWorkspace();
}
