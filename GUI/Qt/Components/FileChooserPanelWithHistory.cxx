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
  m_oracleTarget = NULL;

  // History menu
  QMenu *history = new QMenu("History", ui->btnHistory);
  ui->btnHistory->setMenu(history);

  // Set up an event filter
  ui->inFilename->installEventFilter(this);

  // Connect up the format selector to the filename
  connect(ui->inFormat, SIGNAL(activated(QString)), this, SLOT(setActiveFormat(QString)));
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

void FileChooserPanelWithHistory::parseFilters(const QString &activeFormat)
{
  // Clear the filters
  m_Filter.clear();
  ui->inFormat->clear();

  m_defaultFormat = activeFormat;

  // Split the pattern into pieces
  QStringList pats = m_filePattern.split(";;", QString::SkipEmptyParts);

  // Split each piece
  foreach(QString pat, pats)
    {
    // Handle patterns in parentheses
    QRegExp rx("(.*)\\((.*)\\)");
    int pos = rx.indexIn(pat);
    if(pos >= 0)
      {
      // Split into title and list of extensions
      QString title = rx.cap(1).trimmed();
      QString extliststr = rx.cap(2).trimmed();

      // Store the extension
      QStringList extlist = extliststr.split(" ");
      QStringList extlistclean;

      foreach (QString myext, extlist)
        {
        pos = myext.indexOf(".");
        if(pos >= 0)
          extlistclean.push_back(myext.mid(pos+1));
        }

      // Make sure every title has somethign!
      if(extlistclean.size() == 0)
        extlistclean.push_back(QString());

      // Append this info
      m_Filter[title] = extlistclean;

      // Add the title to the format dropbox
      ui->inFormat->addItem(title);

      // Use this filter
      if(m_defaultFormat.length() == 0)
        m_defaultFormat = title;
      }
    }

  // Update the combo box
  ui->inFormat->setCurrentText(m_defaultFormat);

  // Show or hide the format panel depending on the number of formats available
  ui->panelFormat->setVisible(m_Filter.size() > 1);
}


QString FileChooserPanelWithHistory::fixExtension() const
{
  // This method appends the extension to the currently entered filename if the
  // currently entered filename does not have an extension already. This is so
  // that we can type in test and it gets saved as test.png
  QString filename = ui->inFilename->text();
  QString filenameAbs = this->absoluteFilenameKeepExtension();

  // Cases when we don't append the extension
  if(filename.length() == 0 ||                  // No filename entered
     m_defaultFormat.length() == 0 ||           // No current format selected
     m_Filter[m_defaultFormat].size() == 0 ||   // Current format does not have any extensions
     QFileInfo(filenameAbs).isDir() ||          // Selected filename is a directory
     m_forceExtension == false)                 // User asked not to do this
    return filename;

  // Check if the filename already has one of the extensions for the selected format
  foreach(QString ext, m_Filter[m_defaultFormat])
    {
    if(ext.length())
      {
      QString eext = QString(".%1").arg(ext);
      if(filename.endsWith(eext))
        return filename;
      }
    }

  // Default extension is the first extension in the accepted list
  QString defaultExt = m_Filter[m_defaultFormat].front();

  // Is the thing the user typed in ending with a dot? Avoid having two dots
  if(filename.endsWith("."))
    return filename + defaultExt;

  // Otherwise, return the filename with the default extension
  return filename + "." + defaultExt;
}

void FileChooserPanelWithHistory::initializeForOpenFile(
    GlobalUIModel *model,
    const QString &labelText,
    const QString &historyCategory,
    const QString &filePattern,
    const QString &initialFile,
    const QString &activeFormat)
{
  // State
  m_Model = model;
  m_openMode = true;
  m_directoryMode = false;
  m_filePattern = filePattern;
  m_historyCategory = historyCategory;
  m_forceExtension = false;

  // Compute the suffix
  parseFilters(activeFormat);

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
    const QString &initialFile,
    const QString &activeFormat)
{
  // State
  m_Model = model;
  m_openMode = false;
  m_directoryMode = false;
  m_filePattern = filePattern;
  m_historyCategory = historyCategory;
  m_forceExtension = force_extension;

  // Compute the suffix
  parseFilters(activeFormat);

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
    ui->inFilename->setText(QString("%1.%2").arg(default_basename, m_Filter[m_defaultFormat].front()));
    }

  // Highlight just the filename
  highlightFilename();

  // Update the display
  on_inFilename_textChanged(ui->inFilename->text());
}

void FileChooserPanelWithHistory::highlightFilename()
{
  // Select the part of the filename minus the extension
  QString text = ui->inFilename->text();
  foreach(QString ext, m_Filter[m_defaultFormat])
    {
    if(text.endsWith(ext, Qt::CaseInsensitive))
      {
      ui->inFilename->setSelection(0, text.length() - (1+ext.length()));
      return;
      }
    }
}

void FileChooserPanelWithHistory::addButton(QWidget *button)
{
  ui->wButtonPanel->layout()->addWidget(button);
}

void FileChooserPanelWithHistory::setCustomFormatOracle(QObject *target, const char *slot)
{
  m_oracleTarget = target;
  m_oracleSlot = slot;
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

QString FileChooserPanelWithHistory::absoluteFilenameKeepExtension() const
{
  QFileInfo fi(ui->inFilename->text());
  if(fi.isAbsolute())
    return fi.absoluteFilePath();

  QFileInfo fi2(QDir(m_workingDir), ui->inFilename->text());
  return fi2.absoluteFilePath();
}

QString FileChooserPanelWithHistory::activeFormat() const
{
  return m_defaultFormat;
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

void FileChooserPanelWithHistory::setActiveFormat(QString format)
{
  if(format == m_defaultFormat)
    return;

  // Change the format
  QString oldFormat = m_defaultFormat;
  m_defaultFormat = format;

  // In open mode, we don't tweak the extension
  if(m_openMode)
    return;

  // Get the default new suffix
  QString newSuffix = m_Filter[format].front();

  // If the one of the recognized extensions is currently selected, replace it
  QString fn = ui->inFilename->text();
  foreach (QString ext, m_Filter[oldFormat])
    {
    QString eext = QString(".%1").arg(ext);
    if(fn.endsWith(eext, Qt::CaseInsensitive))
      {
      fn = fn.mid(0, fn.length() - ext.length()) + newSuffix;
      break;
      }
    }

  // Modify the extension if it was not overridden
  if(fn != ui->inFilename->text())
    {
    ui->inFilename->setText(fn);
    }

  // Highlight the filename
  highlightFilename();
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

  // Create a file dialog
  QFileDialog dialog(this, ui->label->text());

  if(m_openMode)
    {
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    }
  else
    {
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    }

  if(browseDir.length())
    {
    QFileInfo file_info(browseDir);
    if(file_info.isDir())
      {
      dialog.setDirectory(file_info.absoluteFilePath() + "/");
      }
    else
      {
      dialog.setDirectory(file_info.absolutePath() + "/");
      dialog.selectFile(file_info.fileName());
      }
    }

  // Create a single filter that combines all of the extensions for all image types
  // If any of the extensions is missing, there will be no filter at all
  QStringList flatExtensionList;
  QStringList formatList;
  QString formatEntry;
  bool have_empty = false;
  foreach(QString format, m_Filter.keys())
    {
    QStringList formatExtensionList;
    foreach(QString ext, m_Filter[format])
      {
#ifdef __APPLE__
      // On MacOS, compound extensions are a problem
      int pos = ext.lastIndexOf(".");
      if(pos >= 0)
        {
        if(m_openMode)
          ext = ext.replace('.','*');
        }
#endif
      if(ext.length())
        {
        QString eext = QString("*.%1").arg(ext);
        flatExtensionList << eext;
        formatExtensionList << eext;
        }
      else
        {
        have_empty = true;
        formatExtensionList << "*";
        }
      }

    QString line = QString("%1 (%2)").arg(format).arg(formatExtensionList.join(" "));
    formatList << line;

    if(m_defaultFormat == format)
      formatEntry = line;
    }

  if(m_openMode)
    {
    QString allext = flatExtensionList.join(" ");
    if(!have_empty && allext.length())
      {
      dialog.setNameFilter(allext);
      }
    }
  else
    {
    dialog.setNameFilters(formatList);
    dialog.selectNameFilter(formatEntry);
    }

  if(dialog.exec() && dialog.selectedFiles().size())
    sel = dialog.selectedFiles().first();

  if(sel.length())
    {
    // Update the selection
    this->updateFilename(sel);
    }
}

QString FileChooserPanelWithHistory::guessFormat(const QString &text)
{
  QString newFormat;

  // Try using the oracle
  if(m_oracleTarget)
    {
    QMetaObject::invokeMethod(m_oracleTarget, m_oracleSlot, Qt::DirectConnection,
                              Q_RETURN_ARG(QString, newFormat),
                              Q_ARG(QString, text));
    }

  // If the oracle failed, try to guess
  if(newFormat.isNull())
    {
    foreach(QString format, m_Filter.keys())
      {
      foreach(QString ext, m_Filter[format])
        {
        QString eext = QString(".%1").arg(ext);
        if(ext.length() && text.endsWith(eext))
          {
          newFormat = format;
          break;
          }
        }
      }
    }

  return newFormat;
}

void FileChooserPanelWithHistory::on_inFilename_textChanged(const QString &text)
{
  // The filename has changed. The first thing we do is to see if the filename has
  // an extension that matches one of our supported extensions. If it does, then
  // we change the active format to be that format
  QString format = guessFormat(absoluteFilenameKeepExtension());
  if(format.length())
    {
    m_defaultFormat = format;
    ui->inFormat->setCurrentText(format);
    emit activeFormatChanged(format);
    }

  else if(m_openMode)
    {
    m_defaultFormat = QString();
    ui->inFormat->setCurrentIndex(-1);
    emit activeFormatChanged(format);
    }

  // At this point the format might have been changed to match the filename
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
    else if(ui->inFormat->currentIndex() == -1 && ui->inFilename->text().length())
      ui->outError->setText("Unable to recognize file format");
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

