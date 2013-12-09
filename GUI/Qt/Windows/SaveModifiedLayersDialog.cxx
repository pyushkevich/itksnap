#include "SaveModifiedLayersDialog.h"
#include "ui_SaveModifiedLayersDialog.h"
#include "LatentITKEventNotifier.h"
#include <QStandardItemModel>
#include <QPushButton>
#include "SaveModifiedLayersModel.h"
#include "SNAPQtCommon.h"

SaveModifiedLayersDialog::SaveModifiedLayersDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::SaveModifiedLayersDialog)
{
  ui->setupUi(this);

  // Resize the table to contents
  ui->tableLayers->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  ui->tableLayers->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
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
}

void SaveModifiedLayersDialog::SetOptions(PromptOptions opts)
{
  // Toggle the discard button
  ui->buttonBox->button(QDialogButtonBox::Discard)->setEnabled(
        !opts.testFlag(DiscardDisabled));
}

bool SaveModifiedLayersDialog::PromptForUnsavedChanges(
    GlobalUIModel *model, PromptOptions opts, QWidget *parent)
{
  // Create a model
  SmartPtr<SaveModifiedLayersModel> saveModel = SaveModifiedLayersModel::New();
  saveModel->SetParentModel(model);

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
  QStandardItemModel *qsim = new QStandardItemModel(items.size(), 2, ui->tableLayers);
  qsim->setHorizontalHeaderItem(0, new QStandardItem("Unsaved Item"));
  qsim->setHorizontalHeaderItem(1, new QStandardItem("Filename"));

  // Add entries for all the items
  for(int i = 0; i < items.size(); i++)
    {
    AbstractSaveableItem *unsaved = items[i];

    // The first column
    QStandardItem *item = new QStandardItem(from_utf8(unsaved->GetDescription()));
    item->setData(unsaved->GetId(), Qt::UserRole);

    qsim->setItem(i, 0, item);
    qsim->setItem(i, 1, new QStandardItem(from_utf8(unsaved->GetFilename())));
    }

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


