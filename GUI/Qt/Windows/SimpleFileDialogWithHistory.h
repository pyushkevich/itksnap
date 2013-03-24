#ifndef SIMPLEFILEDIALOGWITHHISTORY_H
#define SIMPLEFILEDIALOGWITHHISTORY_H

#include <QDialog>

namespace Ui {
class SimpleFileDialogWithHistory;
}

class SimpleFileDialogWithHistory : public QDialog
{
  Q_OBJECT
  
public:

  static QString showOpenDialog(
      QString window_title, QString file_title,
      QStringList &local_history, QStringList &global_history,
      QString file_pattern);
  
  static QString showSaveDialog(
      QString window_title, QString file_title,
      QStringList &local_history, QStringList &global_history,
      QString file_pattern);

private slots:
  void on_btnBrowse_clicked();

  void onHistorySelection();

protected:

  // Constructor is protected, use static methods to launch
  explicit SimpleFileDialogWithHistory(QWidget *parent = 0);
  ~SimpleFileDialogWithHistory();

  // Populate the history
  void populateHistory(const QStringList &local_history,
                       const QStringList &global_history);

  // Mode of operation (Save/Open)
  bool m_OpenMode;

  // Default file pattern
  QString m_FilePattern;

private:
  Ui::SimpleFileDialogWithHistory *ui;

};

#endif // SIMPLEFILEDIALOGWITHHISTORY_H
