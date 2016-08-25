#include "SimpleFileDialogWithHistory.h"
#include "ui_SimpleFileDialogWithHistory.h"
#include "FileChooserPanelWithHistory.h"

#include "SNAPQtCommon.h"

SimpleFileDialogWithHistory::SimpleFileDialogWithHistory(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::SimpleFileDialogWithHistory)
{
  // Use parent's double buffering attributes
  this->setAttribute(Qt::WA_PaintOnScreen, parent->testAttribute(Qt::WA_PaintOnScreen));

  // Set an object name for scripting
  this->setObjectName("dlgSimpleFile");

  ui->setupUi(this);

  // Connect up
  connect(this, SIGNAL(accepted()), ui->filePanel, SLOT(onFilenameAccept()));
}

SimpleFileDialogWithHistory::~SimpleFileDialogWithHistory()
{
  delete ui;
}

SimpleFileDialogWithHistory::QueryResult
SimpleFileDialogWithHistory
::showOpenDialog(QWidget *parent,
                 GlobalUIModel *model,
                 QString window_title,
                 QString file_title,
                 QString history_name,
                 QString file_pattern, QString init_file)
{
  QueryResult result;

  // Configure the dialog
  SimpleFileDialogWithHistory *dialog = new SimpleFileDialogWithHistory(parent);
  dialog->ui->filePanel->initializeForOpenFile(model, file_title, history_name, file_pattern, init_file);
  dialog->setWindowTitle(window_title);

  // Launch the dialog
  if(dialog->exec() == QDialog::Accepted)
    {
    result.filename = dialog->ui->filePanel->absoluteFilename();
    result.activeFormat = dialog->ui->filePanel->activeFormat();
    }

  return result;
}

#include <QMessageBox>

SimpleFileDialogWithHistory::QueryResult
SimpleFileDialogWithHistory::showSaveDialog(QWidget *parent, GlobalUIModel *model,
                 QString window_title,
                 QString file_title,
                 QString history_name,
                 QString file_pattern, bool force_extension,
                 QString init_file)
{
  QueryResult result;

  // Configure the dialog
  SimpleFileDialogWithHistory *dialog = new SimpleFileDialogWithHistory(parent);
  dialog->setWindowTitle(window_title);
  dialog->ui->filePanel->initializeForSaveFile(model, file_title, history_name, file_pattern,
                                               force_extension, init_file);

  // Launch the dialog
  if(dialog->exec() == QDialog::Accepted)
    {
    result.filename = dialog->ui->filePanel->absoluteFilename();
    result.activeFormat = dialog->ui->filePanel->activeFormat();
    }

  return result;
}



