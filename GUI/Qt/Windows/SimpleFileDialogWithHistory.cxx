#include "SimpleFileDialogWithHistory.h"
#include "ui_SimpleFileDialogWithHistory.h"
#include <QFileInfo>
#include <QFileDialog>
#include <QMenu>
#include <QCompleter>
#include <QFileSystemModel>

#include "SNAPQtCommon.h"

SimpleFileDialogWithHistory::SimpleFileDialogWithHistory(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::SimpleFileDialogWithHistory)
{
  // Set an object name for scripting
  this->setObjectName("dlgSimpleFile");

  ui->setupUi(this);
  m_OpenMode = true;

  // History menu
  QMenu *history = new QMenu("History", ui->btnHistory);
  ui->btnHistory->setMenu(history);

  // Set up a completer
  QCompleter *completer = new QCompleter(this);
  QFileSystemModel *fsm = new QFileSystemModel(completer);
  fsm->setResolveSymlinks(true);
  completer->setModel(fsm);
  completer->setCompletionMode(QCompleter::InlineCompletion);
  ui->inFilename->setCompleter(completer);

  // Set up an event filter
  ui->inFilename->installEventFilter(this);
}

SimpleFileDialogWithHistory::~SimpleFileDialogWithHistory()
{
  delete ui;
}

void SimpleFileDialogWithHistory
::populateHistory(const QStringList &local_history,
                  const QStringList &global_history)
{
  QMenu *menu = ui->btnHistory->menu();
  PopulateHistoryMenu(menu, this, SLOT(onHistorySelection()),
                      local_history, global_history);
  ui->btnHistory->setEnabled(menu->actions().size() > 0);
}

QString SimpleFileDialogWithHistory::fixExtension()
{
  QString filename = ui->inFilename->text();
  QFileInfo fi(filename);

  if(QString::compare(fi.suffix(), m_DefaultSuffix, Qt::CaseInsensitive) == 0)
    {
    // The suffix is the same as the default suffix
    return filename;
    }

  // At this point, the user might have typed whatever partial string. We need
  // to make sure that what the user has typed is actually a part of the filename

  // Is the text that the user typed in referring to a directory? Then there is no
  // sense to add an extension to it!
  if(fi.isDir())
    return filename;

  // Is the thing the user typed in ending with a dot?
  if(filename.endsWith("."))
    return filename + m_DefaultSuffix;

  // Otherwise, return the filename with the default extension
  else
    return filename + "." + m_DefaultSuffix;
}

#include <QKeyEvent>

bool SimpleFileDialogWithHistory::eventFilter(QObject *obj, QEvent *ev)
{
  if(obj == ui->inFilename)
    {
    if(ev->type() == QEvent::KeyPress)
      {
      QKeyEvent *keyEvent = static_cast<QKeyEvent *>(ev);
      if(keyEvent->key() == Qt::Key_Tab)
        {
        if(ui->inFilename->completer()->completionCount())
          {
          ui->inFilename->completer()->complete();
          return true;
          }
        }
      }
    }

  return QObject::eventFilter(obj, ev);

}

QString
SimpleFileDialogWithHistory
::showOpenDialog(QWidget *parent,
                 QString window_title,
                 QString file_title,
                 QStringList &local_history, QStringList &global_history,
                 QString file_pattern)
{
  // Configure the dialog
  SimpleFileDialogWithHistory *dialog = new SimpleFileDialogWithHistory(parent);
  dialog->setWindowTitle(window_title);
  dialog->ui->label->setText(file_title);
  dialog->m_OpenMode = true;
  dialog->m_FilePattern = file_pattern;
  dialog->populateHistory(local_history, global_history);

  dialog->ui->outError->clear();
  dialog->ui->outSavePath->clear();

  // Launch the dialog
  if(dialog->exec() == QDialog::Accepted)
    {
    return dialog->ui->inFilename->text();
    }
  else return QString();
}

#include <QMessageBox>

QString
SimpleFileDialogWithHistory
::showSaveDialog(QWidget *parent,
                 QString window_title,
                 QString file_title,
                 QStringList &local_history, QStringList &global_history,
                 QString file_pattern, QString force_extension)
{
  // Configure the dialog
  SimpleFileDialogWithHistory *dialog = new SimpleFileDialogWithHistory(parent);
  dialog->setWindowTitle(window_title);
  dialog->ui->label->setText(file_title);
  dialog->m_OpenMode = false;
  dialog->m_FilePattern = file_pattern;
  dialog->m_DefaultSuffix = force_extension;
  dialog->populateHistory(local_history, global_history);

  dialog->ui->outError->clear();
  dialog->ui->outSavePath->clear();

  // Initialize the dialog with a default filename
  QString default_basename = "Untitled";
  dialog->ui->inFilename->setText(QString("%1.%2").arg(default_basename, force_extension));
  dialog->ui->inFilename->setSelection(0, default_basename.length());

  // Launch the dialog
  if(dialog->exec() == QDialog::Accepted)
    {
    return dialog->fixExtension();
    }
  else return QString();
}


void SimpleFileDialogWithHistory::on_btnBrowse_clicked()
{
  QFileInfo file_info(ui->inFilename->text());
  if(m_OpenMode)
    {
    QString sel =
        GetOpenFileNameBugFix(this, this->windowTitle(),
                              ui->inFilename->text(), m_FilePattern);
    if(sel.length())
      ui->inFilename->setText(sel);
    }
  else
    {
    QString sel = QFileDialog::getSaveFileName(
          this,
          this->windowTitle(),
          file_info.absoluteFilePath(),
          m_FilePattern);
    if(sel.length())
      ui->inFilename->setText(sel);
    }
}

void SimpleFileDialogWithHistory::onHistorySelection()
{
  QAction *action = static_cast<QAction *>(this->sender());
  ui->inFilename->setText(action->text());
}


void SimpleFileDialogWithHistory::on_inFilename_textChanged(const QString &text)
{
  // Changes to the text box will be used to populate the error text
  if(m_OpenMode)
    {
    // Does the file exist?
    QFileInfo fi(text);
    if(!fi.exists())
      ui->outError->setText("File does not exist.");
    else if(!fi.isReadable())
      ui->outError->setText("File is not readable.");
    else
      ui->outError->setText("");
    }
  else
    {
    // Clear the output messages
    ui->outSavePath->setText("");
    ui->outError->setText("");

    if(text.length())
      {
      // Get the extension
      QString saveFile = this->fixExtension();
      QFileInfo fi(saveFile);

      // If the file is a directory, we don't give any errors, as the user is probably just typing
      if(fi.isDir())
        return;

      // For relative paths, inform the user where the file will be saved in
      if(fi.isRelative())
        ui->outSavePath->setText(QString("Saving in %1").arg(fi.absolutePath()));

      // Does the file exist?
      if(fi.exists())
        ui->outError->setText("Existing file will be overridden!");
      }
    }
}
