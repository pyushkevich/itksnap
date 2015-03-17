/*
part of Click'n'Join mode, which was contributed by Roman Grothausmann
*/

#ifndef GLOBALWSWIZARDPANEL_H
#define GLOBALWSWIZARDPANEL_H

#include <QWidget>
#include <QAbstractItemModel>

namespace Ui {
class GlobalWSWizardPanel;
}

class GlobalUIModel;
class GlobalWSWizardModel;
class WatershedPipeline;



class GlobalWSWizardPanel : public QWidget
{
  Q_OBJECT

public:
  explicit GlobalWSWizardPanel(QWidget *parent = 0);
  ~GlobalWSWizardPanel();

  void SetModel(GlobalUIModel *model);

  /**
    Put the panel into the initial state, i.e., ready to perform preprocessing.
    You must call this method before showing the panel.
    */
  void Initialize();

signals:

  void wizardFinished();

private slots:

  void on_stack_currentChanged(int arg1);

  void on_btnCancel_clicked();

  void on_btnNextPreproc_clicked();

  void on_btnWSRangeNext_clicked();

  void on_inWSLevel_valueChanged(double value);

  void on_btnGWSfinish_clicked();

  void on_actionIncreaseWSLevel_triggered();

  void on_actionDecreaseWSLevel_triggered();

  void on_btnCopySeg_clicked();

  void on_btnClearSeg_clicked();

  void on_btnLoadFromFile_clicked();

  void on_btnWSRangeBack_clicked();

  void on_btnJoinBack_clicked();

private:

  GlobalUIModel *m_ParentModel;
  GlobalWSWizardModel *m_Model;

  double m_old_value;

  Ui::GlobalWSWizardPanel *ui;

protected:

  WatershedPipeline *m_Watershed;

};

#endif // GLOBALWSWIZARDPANEL_H
