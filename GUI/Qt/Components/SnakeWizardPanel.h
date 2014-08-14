#ifndef SNAKEWIZARDPANEL_H
#define SNAKEWIZARDPANEL_H

#include <QWidget>
#include <QAbstractItemModel>

namespace Ui {
class SnakeWizardPanel;
}

class SpeedImageDialog;
class GlobalUIModel;
class SnakeWizardModel;
class QTimer;
class SnakeParameterDialog;
class QToolBar;

class BubbleItemModel : public QAbstractTableModel
{
  Q_OBJECT

public:
  BubbleItemModel(QObject *parent) : QAbstractTableModel(parent) {}
  virtual ~BubbleItemModel() {}

  void setSourceModel(SnakeWizardModel *model);

  virtual int rowCount(const QModelIndex &parent) const;
  virtual int columnCount(const QModelIndex &parent) const;
  virtual QVariant data(const QModelIndex &index, int role) const;
  virtual bool setData(const QModelIndex &index, const QVariant &value, int role);

  virtual Qt::ItemFlags flags(const QModelIndex &index) const;

  virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;

public slots:

  void onBubbleListUpdate();
  void onBubbleValuesUpdate();

private:
  SnakeWizardModel *m_Model;
};



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

signals:

  void wizardFinished();

private slots:

  void on_btnNextPreproc_clicked();

  void on_btnAddBubble_clicked();

  void on_btnRemoveBubble_clicked();

  void on_btnBubbleNext_clicked();

  void on_btnBubbleBack_clicked();

  void on_stack_currentChanged(int arg1);

  void on_btnPlay_toggled(bool checked);

  void idleCallback();

  void on_btnSingleStep_clicked();


  void on_btnEvolutionBack_clicked();

  void on_btnEvolutionNext_clicked();

  void on_btnRewind_clicked();

  void on_btnEvolutionParameters_clicked();

  void on_btnCancel_clicked();

  void on_actionIncreaseBubbleRadius_triggered();

  void on_actionDecreaseBubbleRadius_triggered();

  void on_btnClusterIterate_clicked();

  void on_btnClusterIterateMany_clicked();

  void on_btnClusterReinitialize_clicked();

  void on_btnClassifyTrain_clicked();

  void on_btnThreshDetail_clicked();

  void on_btnClusterDetail_clicked();

  void on_btnClassifyClearExamples_clicked();

  // Slot called when a quick-label is selected in the classify pane
  void onClassifyQuickLabelSelection();

  void on_btnEdgeDetail_clicked();

private:

  SpeedImageDialog *m_SpeedDialog;
  SnakeParameterDialog *m_ParameterDialog;
  GlobalUIModel *m_ParentModel;
  SnakeWizardModel *m_Model;

  QTimer *m_EvolutionTimer;

  Ui::SnakeWizardPanel *ui;
};

#endif // SNAKEWIZARDPANEL_H
