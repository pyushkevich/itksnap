#ifndef LOADTRANSFORMATIONDIALOG_H
#define LOADTRANSFORMATIONDIALOG_H

#include <QDialog>

namespace Ui {
class LoadTransformationDialog;
}

class GlobalUIModel;

class LoadTransformationDialog : public QDialog
{
  Q_OBJECT
  
public:

  struct QueryResult
  {
    QString filename;
    QString activeFormat;
    bool compose = false;
    bool inverse = false;
  };

  static QueryResult showDialog(QWidget *parent, GlobalUIModel *model);

protected:

  // Constructor is protected, use static methods to launch
  explicit LoadTransformationDialog(QWidget *parent = 0);
  ~LoadTransformationDialog();

private:
  Ui::LoadTransformationDialog *ui;

};

#endif // LOADTRANSFORMATIONDIALOG_H
