#ifndef SNAKEWIZARDPANEL_H
#define SNAKEWIZARDPANEL_H

#include <QWidget>

namespace Ui {
class SnakeWizardPanel;
}

class SpeedImageDialog;
class GlobalUIModel;
class SnakeWizardModel;

class SnakeWizardPanel : public QWidget
{
  Q_OBJECT

public:
  explicit SnakeWizardPanel(QWidget *parent = 0);
  ~SnakeWizardPanel();

  void SetModel(GlobalUIModel *model);

private slots:

  void on_btnPreprocess_clicked();

private:

  SpeedImageDialog *m_SpeedDialog;
  GlobalUIModel *m_ParentModel;
  SnakeWizardModel *m_Model;

  Ui::SnakeWizardPanel *ui;
};

#endif // SNAKEWIZARDPANEL_H
