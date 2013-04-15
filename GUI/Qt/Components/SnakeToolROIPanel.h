#ifndef SNAKETOOLROIPANEL_H
#define SNAKETOOLROIPANEL_H

#include <SNAPComponent.h>

class GlobalUIModel;
class ResampleDialog;

namespace Ui {
class SnakeToolROIPanel;
}

class SnakeToolROIPanel : public SNAPComponent
{
  Q_OBJECT

public:
  explicit SnakeToolROIPanel(QWidget *parent = 0);
  ~SnakeToolROIPanel();

  void SetModel(GlobalUIModel *);

private slots:
  void on_btnResetROI_clicked();

  void on_btnAuto_clicked();

private:
  Ui::SnakeToolROIPanel *ui;

  GlobalUIModel *m_Model;
  ResampleDialog *m_ResampleDialog;
};

#endif // SNAKETOOLROIPANEL_H
