#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>

namespace Ui {
class PreferencesDialog;
}

class GlobalPreferencesModel;

class PreferencesDialog : public QDialog
{
  Q_OBJECT
  
public:
  explicit PreferencesDialog(QWidget *parent = 0);
  ~PreferencesDialog();

  void SetModel(GlobalPreferencesModel *model);

public slots:
  virtual void show();
  
private:
  Ui::PreferencesDialog *ui;

  GlobalPreferencesModel *m_Model;
};

#endif // PREFERENCESDIALOG_H
