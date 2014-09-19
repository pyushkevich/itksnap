#ifndef SIMPLEFILEDIALOGWITHHISTORY_H
#define SIMPLEFILEDIALOGWITHHISTORY_H

#include <QDialog>

namespace Ui {
class SimpleFileDialogWithHistory;
}

class GlobalUIModel;

class SimpleFileDialogWithHistory : public QDialog
{
  Q_OBJECT
  
public:

  static QString showOpenDialog(
      QWidget *parent,
      GlobalUIModel *model,
      QString window_title,
      QString file_title,
      QString history_name,
      QString file_pattern,
      QString init_file = QString());
  
  static QString showSaveDialog(
      QWidget *parent,
      GlobalUIModel *model,
      QString window_title, QString file_title,
      QString history_name,
      QString file_pattern,
      bool force_extension,
      QString init_file = QString());

protected:

  // Constructor is protected, use static methods to launch
  explicit SimpleFileDialogWithHistory(QWidget *parent = 0);
  ~SimpleFileDialogWithHistory();

private:
  Ui::SimpleFileDialogWithHistory *ui;

};

#endif // SIMPLEFILEDIALOGWITHHISTORY_H
