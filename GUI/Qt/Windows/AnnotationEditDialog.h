#ifndef ANNOTATIONEDITDIALOG_H
#define ANNOTATIONEDITDIALOG_H

#include <QDialog>

class AnnotationModel;

namespace Ui {
class AnnotationEditDialog;
}

/** Simple modal dialog for editing annotations */
class AnnotationEditDialog : public QDialog
{
  Q_OBJECT

public:
  explicit AnnotationEditDialog(QWidget *parent = 0);
  virtual ~AnnotationEditDialog() {}

  void SetModel(AnnotationModel *model);

private:
  Ui::AnnotationEditDialog *ui;
  AnnotationModel *m_Model;
};

#endif // ANNOTATIONEDITDIALOG_H
