#include "FileChooserPanelWithHistory.h"
#include "ui_FileChooserPanelWithHistory.h"

#include <QMenu>
#include <GlobalUIModel.h>
#include <HistoryManager.h>
#include <IRISApplication.h>
#include <QFileInfo>
#include <QKeyEvent>
#include <QDir>
#include <QFileDialog>
#include <SNAPQtCommon.h>


FileChooserPanelWithHistory::FileChooserPanelWithHistory(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FileChooserPanelWithHistory)
{
  ui->setupUi(this);

  // INitialize vars
  m_Model = NULL;

  // History menu
  QMenu *history = new QMenu("History", ui->btnHistory);
  ui->btnHistory->setMenu(history);

  // Set up an event filter
  ui->inFilename->installEventFilter(this);
}

FileChooserPanelWithHistory::~FileChooserPanelWithHistory()
{
  delete ui;
}

void FileChooserPanelWithHistory::populateHistory()
{
  // Get the history string lists
  HistoryManager *hm =
      m_Model->GetDriver()->GetSystemInterface()->GetHistoryManager();

  QStringList local_history =
      toQStringList(hm->GetLocalHistory(m_historyCategory.toStdString()));
  QStringList global_history =
      toQStringList(hm->GetGlobalHistory(m_historyCategory.toStdString()));

  // Fill out the menu
  QMenu *menu = ui->btnHistory->menu();
  PopulateHistoryMenu(menu, this, SLOT(onHistorySelection()),
                      local_history, global_history);
  ui->btnHistory->setEnabled(menu->actions().size() > 0);
}

void FileChooserPanelWithHistory::initDefaultSuffix()
{
  QStringList pats = m_filePattern.split(";;", QString::SkipEmptyParts);
  if(pats.size())
    {
    QString pat = pats.front();

    // Handle patterns in parentheses
    QRegExp rx("\\((.*)\\)");
    int pos = rx.indexIn(pat);
    if(pos >= 0)
      pat = rx.cap(1);

    // Split on space
    QStringList extlist = pat.split(" ");
    if(extlist.length())
      {
      // Get the suffix of the extension
      QString myext = extlist.first();
      pos = myext.indexOf(".");
      if(pos >= 0)
        {
        m_defaultSuffix = myext.mid(pos+1);
        return;
        }
      }
    }

  m_defaultSuffix = QString();
}

QString FileChooserPanelWithHistory::fixExtension() const
{
  QString filename = ui->inFilename->text();
  if(filename.length() == 0 || m_defaultSuffix.length() == 0)
    return filename;

  QFileInfo fi(filename);

  if(QString::compare(fi.suffix(), m_defaultSuffix, Qt::CaseInsensitive) == 0)
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
    return filename + m_defaultSuffix;

  // Otherwise, return the filename with the default extension
  else
    return filename + "." + m_defaultSuffix;

}

void FileChooserPanelWithHistory::initializeForOpenFile(
    GlobalUIModel *model,
    const QString &labelText,
    const QString &historyCategory,
    const QString &filePattern,
    const QString &initialFile)
{
  // State
  m_Model = model;
  m_openMode = true;
  m_directoryMode = false;
  m_filePattern = filePattern;
  m_historyCategory = historyCategory;

  // Compute the suffix
  initDefaultSuffix();

  // Initial UI values
  ui->label->setText(labelText);

  // Populate the history
  this->populateHistory();

  // Clear the error fields
  ui->outError->clear();
  ui->outSavePath->clear();

  // Get the working directory based on history or main image
  m_workingDir = GetFileDialogPath(m_Model, m_historyCategory.toStdString().c_str());

  // Set the initial file
  if(initialFile.length())
    {
    this->updateFilename(initialFile);
    }

  // Update the display
  on_inFilename_textChanged(ui->inFilename->text());
}

void FileChooserPanelWithHistory::initializeForSaveFile(
    GlobalUIModel *model,
    const QString &labelText,
    const QString &historyCategory,
    const QString &filePattern,
    bool force_extension,
    const QString &initialFile)
{
  // State
  m_Model = model;
  m_openMode = false;
  m_directoryMode = false;
  m_filePattern = filePattern;
  m_historyCategory = historyCategory;
  m_forceExtension = force_extension;

  // Compute the suffix
  initDefaultSuffix();

  // Initial UI values
  ui->label->setText(labelText);

  // Populate the history
  this->populateHistory();

  // Clear the error fields
  ui->outError->clear();
  ui->outSavePath->clear();

  // Get the working directory based on history or main image
  m_workingDir = GetFileDialogPath(m_Model, m_historyCategory.toStdString().c_str());

  // Set the initial file
  if(initialFile.length())
    {
    // Update the filename and working directory based on the initial file
    this->updateFilename(initialFile);
    }
  else
    {
    // Initialize the dialog with a default filename
    QString default_basename = "Untitled";
    ui->inFilename->setText(QString("%1.%2").arg(default_basename, m_defaultSuffix));
    ui->inFilename->setSelection(0, default_basename.length());
    }

  // Update the display
  on_inFilename_textChanged(ui->inFilename->text());
}

void FileChooserPanelWithHistory::addButton(QWidget *button)
{
  ui->wButtonPanel->layout()->addWidget(button);
}

void FileChooserPanelWithHistory::onHistorySelection()
{
  // Get the absolute filename
  QAction *action = static_cast<QAction *>(this->sender());
  this->updateFilename(action->text());
}

void FileChooserPanelWithHistory::updateFilename(QString filename)
{
  QFileInfo fi(filename);
  QString new_file;

  // If the filename given is relative, define it relative to the working directory
  if(fi.isRelative())
    fi = QFileInfo(m_workingDir, filename);

  // If the path exists, use it as the new working directory
  if(fi.absoluteDir().exists())
    {
    m_workingDir = fi.absolutePath();
    new_file = fi.fileName();
    }
  else
    {
    new_file = fi.absoluteFilePath();
    }

  // Make sure the update code executes even if the text is not changed
  if(new_file == ui->inFilename->text())
    on_inFilename_textChanged(new_file);
  else
    ui->inFilename->setText(new_file);

}

QString FileChooserPanelWithHistory::absoluteFilename() const
{
  QString fix_ext = this->fixExtension();
  QFileInfo fi(fix_ext);
  if(fi.isAbsolute())
    return fi.absoluteFilePath();

  QFileInfo fi2(QDir(m_workingDir), fix_ext);
  return fi2.absoluteFilePath();
}

QString FileChooserPanelWithHistory::errorText() const
{
  return ui->outError->text();
}

void FileChooserPanelWithHistory::setErrorText(const QString &text)
{
  ui->outError->setText(text);
}

void FileChooserPanelWithHistory::onFilenameAccept()
{
  QDir myDir = QFileInfo(this->absoluteFilename()).absoluteDir();
  if(myDir.exists())
    UpdateFileDialogPathForCategory(m_historyCategory.toStdString().c_str(),
                                    myDir.absolutePath());
}

void FileChooserPanelWithHistory::on_btnBrowse_clicked()
{
  // Get the file name
  QString sel;

  // Where to open the dialog?
  QFileInfo bfi(QDir(m_workingDir), ui->inFilename->text());
  if(!bfi.absoluteDir().exists())
    bfi = QFileInfo(m_workingDir);

  QString browseDir = bfi.absoluteFilePath();

  if(m_openMode)
    {
    sel = GetOpenFileNameBugFix(this, ui->label->text(), browseDir, m_filePattern);
    }
  else
    {
    sel = QFileDialog::getSaveFileName(this, ui->label->text(), browseDir, m_filePattern);
    }

  if(sel.length())
    {
    // Update the selection
    this->updateFilename(sel);
    }
}

void FileChooserPanelWithHistory::on_inFilename_textChanged(const QString &text)
{
  // Get the fileinfo for this file
    QString file_ext = this->fixExtension();
  QFileInfo fi(file_ext), fiwd;
  if(fi.isRelative())
    fiwd = QFileInfo(m_workingDir, file_ext);
  else
    fiwd = fi;

  // Clear the output messages
  ui->outSavePath->clear();
  ui->outError->clear();

  // Changes to the text box will be used to populate the error text
  if(m_openMode)
    {
    // Does the file exist?
    if(text.length() && !fiwd.exists())
      ui->outError->setText("The file does not exist");
    else if(text.length() && !fiwd.isReadable())
      ui->outError->setText("The file is not readable");
    else
      ui->outError->setText("");

    // For relative paths, inform the user where the file will be saved in
    if(fi.isRelative() && !fi.isDir())
      {
      QString saveDir = fiwd.isDir() ? fiwd.absoluteFilePath() : fiwd.absolutePath();
      ui->outSavePath->setText(QString("Path: %1").arg(saveDir));
      }
    }
  else
    {
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
        {
        QString saveDir = fiwd.isDir() ? fiwd.absoluteFilePath() : fiwd.absolutePath();
        ui->outSavePath->setText(QString("Path: %1").arg(saveDir));
        }

      // Does the file exist?
      if(fiwd.exists())
        ui->outError->setText("Existing file will be overridden!");
      }
    }

  emit absoluteFilenameChanged(absoluteFilename());
}

bool FileChooserPanelWithHistory::eventFilter(QObject *obj, QEvent *ev)
{
  /*
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
    } */

  return QObject::eventFilter(obj, ev);

}

