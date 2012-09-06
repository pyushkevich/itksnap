#ifndef SNAKEWIZARDPANEL_H
#define SNAKEWIZARDPANEL_H

#include <QWidget>

namespace Ui {
class SnakeWizardPanel;
}

class SpeedImageDialog;
class GlobalUIModel;
class SnakeWizardModel;
class QTimer;

class SnakeWizardPanel : public QWidget
{
  Q_OBJECT

public:
  explicit SnakeWizardPanel(QWidget *parent = 0);
  ~SnakeWizardPanel();

  void SetModel(GlobalUIModel *model);

  /**
    Put the panel into the initial state, i.e., ready to perform preprocessing.
    You must call this method before showing the panel.
    */
  void Initialize();

private slots:

  void on_btnPreprocess_clicked();

  void on_btnNextPreproc_clicked();

  void on_btnAddBubble_clicked();

  void on_btnRemoveBubble_clicked();

  void on_btnBubbleNext_clicked();

  void on_btnBubbleBack_clicked();

  void on_stack_currentChanged(int arg1);

  void on_btnPlay_clicked(bool checked);

  void idleCallback();

  void on_btnSingleStep_clicked();

private:

  SpeedImageDialog *m_SpeedDialog;
  GlobalUIModel *m_ParentModel;
  SnakeWizardModel *m_Model;

  QTimer *m_EvolutionTimer;

  Ui::SnakeWizardPanel *ui;
};

#endif // SNAKEWIZARDPANEL_H
