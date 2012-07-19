#ifndef LABELEDITORDIALOG_H
#define LABELEDITORDIALOG_H

#include <QDialog>

class LabelEditorModel;

namespace Ui {
    class LabelEditorDialog;
}

class LabelEditorDialog : public QDialog
{
  Q_OBJECT

public:
  explicit LabelEditorDialog(QWidget *parent = 0);
  ~LabelEditorDialog();

  void SetModel(LabelEditorModel *model);

private:
  Ui::LabelEditorDialog *ui;

  LabelEditorModel *m_Model;
};

#endif // LABELEDITORDIALOG_H
