#include "SaveModifiedLayersDialog.h"
#include "ui_SaveModifiedLayersDialog.h"
#include "LatentITKEventNotifier.h"
#include <QStandardItemModel>
#include <QPushButton>
#include "SaveModifiedLayersModel.h"
#include "SNAPQtCommon.h"
#include "ImageIOWizard.h"
#include "QtAbstractItemViewCoupling.h"
#include "QtWidgetActivator.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "IRISImageData.h"

class QtSaveModifiedLayersInteractionDelegate
    : public AbstractSaveModifiedLayersInteractionDelegate
{
public:

  virtual bool SaveImageLayer(
      GlobalUIModel *model, ImageWrapperBase *wrapper, LayerRole role)
  {
    return ::SaveImageLayer(model, wrapper, role);
  }

  virtual bool SaveProject(GlobalUIModel *model)
  {
    // TODO: passing NULL here makes this dialog unscriptable!
    return ::SaveWorkspace(NULL, model, false, NULL);
  }

};


SaveModifiedLayersDialog::SaveModifiedLayersDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::SaveModifiedLayersDialog)
{
  ui->setupUi(this);

  // Give the dialog a name
  this->setObjectName("dlgSaveModified");

  // Resize the table to contents
  ui->tableLayers->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tableLayers->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

SaveModifiedLayersDialog::~SaveModifiedLayersDialog()
{
  delete ui;
}

void SaveModifiedLayersDialog::SetModel(SaveModifiedLayersModel *model)
{
  // Save the model
  m_Model = model;

  // Listen to the events from the model
  LatentITKEventNotifier::connect(m_Model, ModelUpdateEvent(),
                                  this, SLOT(onModelUpdate(EventBucket)));

  // Fill out
  this->UpdateUnsavedTable();

  // Couple the model to the list
  makeCoupling((QAbstractItemView *) ui->tableLayers, m_Model->GetCurrentItemModel());

  // Activate the save all button
  activateOnFlag(ui->buttonBox->button(QDialogButtonBox::SaveAll),
                 m_Model, SaveModifiedLayersModel::UIF_CAN_SAVE_ALL);

  // Activate the save all button
  activateOnFlag(ui->buttonBox->button(QDialogButtonBox::Save),
                 m_Model, SaveModifiedLayersModel::UIF_CAN_SAVE_CURRENT);
}

void SaveModifiedLayersDialog::SetOptions(PromptOptions opts)
{
  // Toggle the discard button
  ui->buttonBox->button(QDialogButtonBox::Discard)->setEnabled(
        !opts.testFlag(DiscardDisabled));
}

bool SaveModifiedLayersDialog
::PromptForUnsavedChanges(GlobalUIModel *model, PromptOptions opts, QWidget *parent)
{
  // Call internal method with empty list
  std::list<ImageWrapperBase *> layerlist;
  return PromptForUnsavedChangesInternal(model, layerlist, opts, parent);
}

bool
SaveModifiedLayersDialog
::PromptForUnsavedChanges(
    GlobalUIModel *model, ImageWrapperBase *singleLayer,
    PromptOptions opts, QWidget *parent)
{
  std::list<ImageWrapperBase *> layerlist;
  layerlist.push_back(singleLayer);

  // Show the dialog
  return PromptForUnsavedChangesInternal(model, layerlist, opts, parent);
}

bool SaveModifiedLayersDialog
::PromptForUnsavedChangesInternal(
    GlobalUIModel *model,
    std::list<ImageWrapperBase *> layers,
    PromptOptions opts,
    QWidget *parent)
{
  // Create a callback delegate
  QtSaveModifiedLayersInteractionDelegate cb_delegate;

  // Create and configure the model
  SmartPtr<SaveModifiedLayersModel> saveModel = SaveModifiedLayersModel::New();
  saveModel->Initialize(model, layers);
  saveModel->SetUIDelegate(&cb_delegate);

  // Check if there is anything to save
  if(saveModel->GetUnsavedItems().size() == 0)
    return true;

  // Configure the dialog
  SaveModifiedLayersDialog *dialog = new SaveModifiedLayersDialog(parent);
  dialog->SetModel(saveModel);
  dialog->setModal(true);
  dialog->SetOptions(opts);

  // Show the dialog
  return (dialog->exec() == QDialog::Accepted);
}

bool SaveModifiedLayersDialog
::PromptForUnsavedChanges(GlobalUIModel *model, int role_filter, PromptOptions opts, QWidget *parent)
{
  LayerIterator it = model->GetDriver()->GetIRISImageData()->GetLayers(role_filter);
  std::list<ImageWrapperBase *> layers;
  for(; !it.IsAtEnd(); ++it)
    layers.push_back(it.GetLayer());

  return PromptForUnsavedChangesInternal(model, layers, opts, parent);
}

bool
SaveModifiedLayersDialog
::PromptForUnsavedSegmentationChanges(GlobalUIModel *model)
{
  ImageWrapperBase *layer = model->GetDriver()->GetIRISImageData()->GetSegmentation();
  return PromptForUnsavedChanges(model, layer);
}

void SaveModifiedLayersDialog::onModelUpdate(const EventBucket &bucket)
{
  // We need to update the table
  UpdateUnsavedTable();
}

void SaveModifiedLayersDialog::UpdateUnsavedTable()
{
  typedef SaveModifiedLayersModel::SaveableItemPtrArray ItemArray;
  typedef ItemArray::const_iterator ItemIterator;

  // Make sure the model updates
  m_Model->Update();

  // Get the items
  const ItemArray &items = m_Model->GetUnsavedItems();

  // Create a standard item model for unsaved items
  QStandardItemModel *qsim = new QStandardItemModel(ui->tableLayers);
  qsim->setColumnCount(2);
  qsim->setHorizontalHeaderItem(0, new QStandardItem("Unsaved Item"));
  qsim->setHorizontalHeaderItem(1, new QStandardItem("Filename"));

  // Add entries for all the items
  for(int i = 0; i < items.size(); i++)
    {
    AbstractSaveableItem *unsaved = items[i];
    if(unsaved->NeedsDecision())
      {
      QList<QStandardItem *> row;

      // The first column
      row.append(new QStandardItem(from_utf8(unsaved->GetDescription())));
      row.back()->setData(unsaved->GetId(), Qt::UserRole);

      if(unsaved->GetFilename().size())
        row.append(new QStandardItem(from_utf8(unsaved->GetFilename())));
      else
        row.append(new QStandardItem("Not assigned"));

      qsim->appendRow(row);
      }
    }

  // If there are no more rows to add, we are done with the dialog
  if(qsim->rowCount() == 0)
    this->accept();

  // Before setting the contents, we need to check what's the selected item
  // because the coupling is going to change that. This is actually a bit of
  // an issue with the way we are using tables and couplings right now.
  int item = m_Model->GetCurrentItem();

  // Qt docs say to manually discard the selection model
  QItemSelectionModel *selm = ui->tableLayers->selectionModel();

  // Set the table contents
  ui->tableLayers->setModel(qsim);

  // Qt docs say to manually discard the selection model
  delete selm;

  // Update the current item - might have changed
  m_Model->SetCurrentItem(item);
}



void SaveModifiedLayersDialog::on_buttonBox_clicked(QAbstractButton *button)
{
  // Save selected item
  if(button == ui->buttonBox->button(QDialogButtonBox::Save))
    {
    // If the item has a filename, save it as is. If it does not, save it via
    // its wizard. The trouble is, the wizard needs to know about the item, so
    // this is not so simple.
    m_Model->SaveCurrent();
    }
  else if(button == ui->buttonBox->button(QDialogButtonBox::Discard))
    {
    // Discard the changes to this image.
    m_Model->DiscardCurrent();
    }
  else if(button == ui->buttonBox->button(QDialogButtonBox::SaveAll))
    {
    // Cancel out the dialog.
    m_Model->SaveAll();
    }
  else if(button == ui->buttonBox->button(QDialogButtonBox::Cancel))
    {
    // Cancel out the dialog.
    this->reject();
    }
}
