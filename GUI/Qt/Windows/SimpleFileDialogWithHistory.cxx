#include "SimpleFileDialogWithHistory.h"
#include "ui_SimpleFileDialogWithHistory.h"
#include "FileChooserPanelWithHistory.h"

#include "SNAPQtCommon.h"

SimpleFileDialogWithHistory::SimpleFileDialogWithHistory(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::SimpleFileDialogWithHistory)
{
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

QString
SimpleFileDialogWithHistory
::showOpenDialog(QWidget *parent,
                 GlobalUIModel *model,
                 QString window_title,
                 QString file_title,
                 QString history_name,
                 QString file_pattern, QString init_file)
{
  // Configure the dialog
  SimpleFileDialogWithHistory *dialog = new SimpleFileDialogWithHistory(parent);
  dialog->ui->filePanel->initializeForOpenFile(model, file_title, history_name, file_pattern, init_file);
  dialog->setWindowTitle(window_title);

  // Launch the dialog
  if(dialog->exec() == QDialog::Accepted)
    {
    return dialog->ui->filePanel->absoluteFilename();
    }
  else return QString();
}

#include <QMessageBox>

QString
SimpleFileDialogWithHistory
::showSaveDialog(QWidget *parent, GlobalUIModel *model,
                 QString window_title,
                 QString file_title,
                 QString history_name,
                 QString file_pattern, bool force_extension,
                 QString init_file)
{
  // Configure the dialog
  SimpleFileDialogWithHistory *dialog = new SimpleFileDialogWithHistory(parent);
  dialog->setWindowTitle(window_title);
  dialog->ui->filePanel->initializeForSaveFile(model, file_title, history_name, file_pattern,
                                               force_extension, init_file);

  // Launch the dialog
  if(dialog->exec() == QDialog::Accepted)
    {
    return dialog->ui->filePanel->absoluteFilename();
    }
  else return QString();
}



