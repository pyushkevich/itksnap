#ifndef SNAKEPARAMETERDIALOG_H
#define SNAKEPARAMETERDIALOG_H

#include <QDialog>
#include <SNAPCommon.h>

namespace Ui {
class SnakeParameterDialog;
}

class SnakeParameterModel;
class EventBucket;
class SnakeParameterPreviewRenderer;

class SnakeParameterDialog : public QDialog
{
  Q_OBJECT
  
public:
  explicit SnakeParameterDialog(QWidget *parent = 0);
  ~SnakeParameterDialog();

  void SetModel(SnakeParameterModel *model);
  
public slots:

  void onModelUpdate(const EventBucket &);

private:
  Ui::SnakeParameterDialog *ui;

  SnakeParameterModel *m_Model;
  SmartPtr<SnakeParameterPreviewRenderer> m_PreviewRenderer[4];
};

#endif // SNAKEPARAMETERDIALOG_H
