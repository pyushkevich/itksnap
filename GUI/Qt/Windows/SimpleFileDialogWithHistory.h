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
      QWidget *parent,
      QString window_title, QString file_title,
      QStringList &local_history, QStringList &global_history,
      QString file_pattern);
  
  static QString showSaveDialog(
      QWidget *parent,
      QString window_title, QString file_title,
      QStringList &local_history, QStringList &global_history,
      QString file_pattern, QString force_extension);

private slots:
  void on_btnBrowse_clicked();

  void onHistorySelection();

  void on_inFilename_textChanged(const QString &arg1);

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

  // Default extension to use if user does not give one
  QString m_DefaultSuffix;

  // Fix the extension of the file in the dialog
  QString fixExtension();

  virtual bool eventFilter(QObject *obj, QEvent *ev);

private:
  Ui::SimpleFileDialogWithHistory *ui;

};

#endif // SIMPLEFILEDIALOGWITHHISTORY_H
