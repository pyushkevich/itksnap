#include "SimpleFileDialogWithHistory.h"
#include "ui_SimpleFileDialogWithHistory.h"
#include <QFileInfo>
#include <QFileDialog>
#include <QMenu>

#include "SNAPQtCommon.h"

SimpleFileDialogWithHistory::SimpleFileDialogWithHistory(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::SimpleFileDialogWithHistory)
{
  ui->setupUi(this);
  m_OpenMode = true;

  // History menu
  QMenu *history = new QMenu("History", ui->btnHistory);
  ui->btnHistory->setMenu(history);
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

QString
SimpleFileDialogWithHistory
::showOpenDialog(QString window_title,
                 QString file_title,
                 QStringList &local_history, QStringList &global_history,
                 QString file_pattern)
{
  // Configure the dialog
  SimpleFileDialogWithHistory *dialog = new SimpleFileDialogWithHistory();
  dialog->setWindowTitle(window_title);
  dialog->ui->label->setText(file_title);
  dialog->m_OpenMode = true;
  dialog->m_FilePattern = file_pattern;
  dialog->populateHistory(local_history, global_history);

  // Launch the dialog
  if(dialog->exec() == QDialog::Accepted)
    {
    return dialog->ui->inFilename->text();
    }
  else return QString();
}

QString
SimpleFileDialogWithHistory
::showSaveDialog(QString window_title,
                 QString file_title,
                 QStringList &local_history, QStringList &global_history,
                 QString file_pattern)
{
  // Configure the dialog
  SimpleFileDialogWithHistory *dialog = new SimpleFileDialogWithHistory();
  dialog->setWindowTitle(window_title);
  dialog->ui->label->setText(file_title);
  dialog->m_OpenMode = false;
  dialog->m_FilePattern = file_pattern;
  dialog->populateHistory(local_history, global_history);

  // Launch the dialog
  if(dialog->exec() == QDialog::Accepted)
    {
    return dialog->ui->inFilename->text();
    }
  else return QString();
}

void SimpleFileDialogWithHistory::on_btnBrowse_clicked()
{
  QFileInfo file_info(ui->inFilename->text());
  if(m_OpenMode)
    {
    QString sel = QFileDialog::getOpenFileName(
          this, this->windowTitle(), file_info.path(), m_FilePattern);
    if(sel.length())
      ui->inFilename->setText(sel);
    }
  else
    {
    QString sel = QFileDialog::getSaveFileName(
          this, this->windowTitle(), file_info.path(), m_FilePattern);
    if(sel.length())
      ui->inFilename->setText(sel);
    }
}

void SimpleFileDialogWithHistory::onHistorySelection()
{
  QAction *action = static_cast<QAction *>(this->sender());
  ui->inFilename->setText(action->text());
}

