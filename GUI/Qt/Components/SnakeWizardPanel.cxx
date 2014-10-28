#include "SnakeWizardPanel.h"
#include "ui_SnakeWizardPanel.h"
#include "SpeedImageDialog.h"
#include "SnakeParameterDialog.h"
#include "GlobalUIModel.h"
#include "SnakeWizardModel.h"
#include "QtCursorOverride.h"
#include "QtComboBoxCoupling.h"
#include "QtWidgetActivator.h"
#include "QtDoubleSliderWithEditorCoupling.h"
#include <QtAbstractItemViewCoupling.h>
#include <QtPagedWidgetCoupling.h>
#include "QtSpinBoxCoupling.h"
#include "QtDoubleSpinBoxCoupling.h"
#include "QtSliderCoupling.h"
#include "QtRadioButtonCoupling.h"
#include "ColorLabelQuickListWidget.h"
#include "IRISException.h"
#include <QMessageBox>
#include <QTimer>
#include <IRISApplication.h>
#include <QToolBar>

Q_DECLARE_METATYPE(PreprocessingMode)

/**
 * An item model for editing bubble data
 */
void BubbleItemModel::setSourceModel(SnakeWizardModel *model)
{
  m_Model = model;
  LatentITKEventNotifier::connect(
        model, SnakeWizardModel::BubbleListUpdateEvent(),
        this, SLOT(onBubbleListUpdate()));
  LatentITKEventNotifier::connect(
        model, SnakeWizardModel::BubbleDefaultRadiusUpdateEvent(),
        this, SLOT(onBubbleValuesUpdate()));
}

int BubbleItemModel::rowCount(const QModelIndex &parent) const
{
  // Only top-level items exist
  if(parent.isValid())
    {
    return 0;
    }
  else
    {
    std::vector<Bubble> &ba = m_Model->GetParent()->GetDriver()->GetBubbleArray();
    return ba.size();
    }
}

int BubbleItemModel::columnCount(const QModelIndex &parent) const
{
  return 4;
}

QVariant BubbleItemModel::data(const QModelIndex &index, int role) const
{
  std::vector<Bubble> &ba = m_Model->GetParent()->GetDriver()->GetBubbleArray();
  Bubble &b = ba[index.row()];
  if(role == Qt::EditRole || role == Qt::DisplayRole)
    {
    if(index.column()==3)
      return QString("%1").arg(b.radius);
    else
      return QString("%1").arg(b.center[index.column()]+1);
    }
  else if(role == Qt::UserRole)
    {
    // This is so that the selection model coupling works
    return index.row();
    }
  return QVariant();
}

bool BubbleItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  std::vector<Bubble> &ba = m_Model->GetParent()->GetDriver()->GetBubbleArray();
  Bubble b = ba[index.row()];
  bool modified = false;

  if(index.column()==3)
    {
    bool convok;
    double dv = value.toDouble(&convok);
    if(convok && dv > 0 && b.radius != dv)
      {
      b.radius = dv;
      modified = true;
      }
    }
  else
    {
    bool convok;
    int iv = value.toInt(&convok);
    if(convok && b.center[index.column()]+1 != iv)
      {
      b.center[index.column()] = iv-1;
      modified = true;
      }
    }

  if(modified)
    return m_Model->UpdateBubble(index.row(), b);

  return false;
}

Qt::ItemFlags BubbleItemModel::flags(const QModelIndex &index) const
{
  Qt::ItemFlags flags =
      Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled;
  return flags;
}

QVariant BubbleItemModel::headerData(
    int section, Qt::Orientation orientation, int role) const
{
  if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
    switch(section)
      {
      case 0 : return tr("X");
      case 1 : return tr("Y");
      case 2 : return tr("Z");
      case 3 : return tr("Radius");
      }
    }
  return QVariant();
}

void BubbleItemModel::onBubbleListUpdate()
{
  emit layoutChanged();
}

void BubbleItemModel::onBubbleValuesUpdate()
{
  emit dataChanged(this->index(0,0),
                   this->index(this->rowCount(QModelIndex())-1,3));
}



SnakeWizardPanel::SnakeWizardPanel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SnakeWizardPanel)
{
  ui->setupUi(this);

  m_SpeedDialog = new SpeedImageDialog(this);
  m_ParameterDialog = new SnakeParameterDialog(this);

  // Hook up the timer!
  m_EvolutionTimer = new QTimer(this);
  connect(m_EvolutionTimer, SIGNAL(timeout()), this, SLOT(idleCallback()));

  // Hook up the quick label selector
  connect(ui->boxLabelQuickList, SIGNAL(actionTriggered(QAction *)),
          this, SLOT(onClassifyQuickLabelSelection()));

  // Adjust the shortcuts for increase/decrease behavior
  ui->actionIncreaseBubbleRadius->setShortcuts(
        ui->actionIncreaseBubbleRadius->shortcuts() << QKeySequence('='));

  ui->actionDecreaseBubbleRadius->setShortcuts(
        ui->actionDecreaseBubbleRadius->shortcuts() << QKeySequence('_'));

  this->addAction(ui->actionIncreaseBubbleRadius);
  this->addAction(ui->actionDecreaseBubbleRadius);
}

SnakeWizardPanel::~SnakeWizardPanel()
{
  delete ui;
}

#include "QtToolbarCoupling.h"

void SnakeWizardPanel::SetModel(GlobalUIModel *model)
{
  // Store and pass on the models
  m_ParentModel = model;
  m_Model = model->GetSnakeWizardModel();
  m_SpeedDialog->SetModel(m_Model);
  m_ParameterDialog->SetModel(model->GetSnakeParameterModel());

  // Couple widgets to models
  makeCoupling(ui->inBubbleRadius, m_Model->GetBubbleRadiusModel());


  // Couple the preprocessing mode combo box
  makeCoupling(ui->inPreprocessMode, m_Model->GetPreprocessingModeModel());

  // Couple the preprocessing mode stack
  std::map<PreprocessingMode, QWidget *> preproc_page_map;
  preproc_page_map[PREPROCESS_THRESHOLD] = ui->pgThreshold;
  preproc_page_map[PREPROCESS_EDGE] = ui->pgEdge;
  preproc_page_map[PREPROCESS_GMM] = ui->pgCluster;
  preproc_page_map[PREPROCESS_RF] = ui->pgClassify;
  preproc_page_map[PREPROCESS_NONE] = ui->pgUserSpeed;
  makePagedWidgetCoupling(ui->stackPreprocess, m_Model->GetPreprocessingModeModel(),
                          preproc_page_map);

  // Couple the thresholding controls
  makeCoupling(ui->inThreshLowerSlider, m_Model->GetThresholdLowerModel());
  makeCoupling(ui->inThreshLowerSpin, m_Model->GetThresholdLowerModel());
  makeCoupling(ui->inThreshUpperSlider, m_Model->GetThresholdUpperModel());
  makeCoupling(ui->inThreshUpperSpin, m_Model->GetThresholdUpperModel());
  makeRadioGroupCoupling(ui->grpThreshMode, m_Model->GetThresholdModeModel());

  // Activation of thresholding controls
  activateOnFlag(ui->inThreshLowerSlider, m_Model, SnakeWizardModel::UIF_LOWER_THRESHOLD_ENABLED);
  activateOnFlag(ui->inThreshLowerSpin, m_Model, SnakeWizardModel::UIF_LOWER_THRESHOLD_ENABLED);
  activateOnFlag(ui->inThreshUpperSlider, m_Model, SnakeWizardModel::UIF_UPPER_THRESHOLD_ENABLED);
  activateOnFlag(ui->inThreshUpperSpin, m_Model, SnakeWizardModel::UIF_UPPER_THRESHOLD_ENABLED);

  // Couple the clustering controls
  makeCoupling(ui->inClusterCount, m_Model->GetNumberOfClustersModel());
  makeCoupling(ui->inClusterActive, m_Model->GetForegroundClusterModel());

  // Couple the classification controls
  makeCoupling(ui->inClassifyForeground, m_Model->GetForegroundClassColorLabelModel());
  makeCoupling(ui->inClassifyTrees, m_Model->GetForestSizeModel());

  // Couple the edge preprocessing controls
  makeCoupling(ui->inEdgeScale, m_Model->GetEdgePreprocessingSigmaModel());
  makeCoupling(ui->inEdgeScaleSlider, m_Model->GetEdgePreprocessingSigmaModel());

  // Initialize the label quick list
  ui->boxLabelQuickList->SetModel(m_Model->GetParent());

  // Set up a model for the bubbles table
  BubbleItemModel *biModel = new BubbleItemModel(this);
  biModel->setSourceModel(m_Model);
  ui->tableBubbleList->setModel(biModel);

  makeCoupling((QAbstractItemView *) ui->tableBubbleList,
               m_Model->GetActiveBubbleModel());

  makeCoupling(ui->inStepSize, m_Model->GetStepSizeModel());
  makeCoupling(ui->outIteration, m_Model->GetEvolutionIterationModel());

  // Activation flags
  /*
  activateOnNotFlag(ui->pgPreproc, m_Model,
                    SnakeWizardModel::UIF_PREPROCESSING_ACTIVE);
                    */

  activateOnFlag(ui->btnNextPreproc, m_Model,
                 SnakeWizardModel::UIF_CAN_GENERATE_SPEED);

  activateOnFlag(ui->btnRemoveBubble, m_Model,
                 SnakeWizardModel::UIF_BUBBLE_SELECTED);

  activateOnFlag(ui->btnBubbleNext, m_Model,
                 SnakeWizardModel::UIF_INITIALIZATION_VALID);
}

void SnakeWizardPanel::Initialize()
{
  // Initialize the model
  m_Model->OnSnakeModeEnter();

  // Go to the right page
  ui->stack->setCurrentWidget(ui->pgPreproc);
}

void SnakeWizardPanel::on_btnNextPreproc_clicked()
{
  // Compute the speed image
  m_Model->ApplyPreprocessing();

  // Finish preprocessing
  m_Model->CompletePreprocessing();

  // Initialize the model
  m_Model->OnBubbleModeEnter();

  // Move to the bubble page
  ui->stack->setCurrentWidget(ui->pgBubbles);
}

void SnakeWizardPanel::on_btnAddBubble_clicked()
{
  m_Model->AddBubbleAtCursor();
}

void SnakeWizardPanel::on_btnRemoveBubble_clicked()
{
  m_Model->RemoveBubbleAtCursor();
}

void SnakeWizardPanel::on_btnBubbleNext_clicked()
{
  // Call the initialization code
  try
  {
    // Handle cursor
    QtCursorOverride curse(Qt::WaitCursor);

    // Initialize the evolution layers
    m_Model->OnEvolutionPageEnter();

    // Move to the evolution page
    ui->stack->setCurrentWidget(ui->pgEvolution);
  }
  catch(IRISException &exc)
  {
    QMessageBox::warning(this, "ITK-SNAP", exc.what(), QMessageBox::Ok);
  }
}

void SnakeWizardPanel::on_btnBubbleBack_clicked()
{
  // Move to the preprocessing page
  ui->stack->setCurrentWidget(ui->pgPreproc);

  // Tell the model
  m_Model->OnBubbleModeBack();
}

void SnakeWizardPanel::on_stack_currentChanged(int page)
{
  // The stack at the top follows the stack at the bottom
  ui->stackStepInfo->setCurrentIndex(page);
}

void SnakeWizardPanel::on_btnPlay_toggled(bool checked)
{
  // This is where we toggle the snake evolution!
  if(checked)
    m_EvolutionTimer->start(10);
  else
    m_EvolutionTimer->stop();
}

void SnakeWizardPanel::idleCallback()
{
  // Step the snake. If converged (returns true), stop playing
  if(m_Model->PerformEvolutionStep())
    ui->btnPlay->setChecked(false);
}

void SnakeWizardPanel::on_btnSingleStep_clicked()
{
  // Turn off the play button (will turn off the timer too)
  ui->btnPlay->setChecked(false);

  // Perform a single step
  m_Model->PerformEvolutionStep();
}


void SnakeWizardPanel::on_btnEvolutionBack_clicked()
{
  // Turn off the play button (will turn off the timer too)
  ui->btnPlay->setChecked(false);

  // Tell the model to return to initialization state
  m_Model->OnEvolutionPageBack();

  // Flip to previous page
  ui->stack->setCurrentWidget(ui->pgBubbles);
}

void SnakeWizardPanel::on_btnEvolutionNext_clicked()
{
  // Turn off the play button (will turn off the timer too)
  ui->btnPlay->setChecked(false);

  // Tell the model to return to initialization state
  m_Model->OnEvolutionPageFinish();

  // Tell parent to hide this window
  emit wizardFinished();
}

void SnakeWizardPanel::on_btnRewind_clicked()
{
  // Turn off the play button (will turn off the timer too)
  ui->btnPlay->setChecked(false);

  // Tell the model to return to initialization state
  m_Model->RewindEvolution();
}

void SnakeWizardPanel::on_btnEvolutionParameters_clicked()
{
  m_ParameterDialog->show();
  m_ParameterDialog->activateWindow();
  m_ParameterDialog->raise();
}

void SnakeWizardPanel::on_btnCancel_clicked()
{
  // Turn off the play button (will turn off the timer too)
  ui->btnPlay->setChecked(false);

  // Make sure all dialogs are closed
  m_SpeedDialog->close();
  m_ParameterDialog->close();

  // Tell the model to return to initialization state
  m_Model->OnCancelSegmentation();

  // Tell parent to hide this window
  emit wizardFinished();
}

void SnakeWizardPanel::on_actionIncreaseBubbleRadius_triggered()
{
  ui->inBubbleRadius->stepUp();
}

void SnakeWizardPanel::on_actionDecreaseBubbleRadius_triggered()
{
  ui->inBubbleRadius->stepDown();
}

void SnakeWizardPanel::on_btnClusterIterate_clicked()
{
  m_Model->PerformClusteringIteration();
}

void SnakeWizardPanel::on_btnClusterIterateMany_clicked()
{
  for(int i = 0; i < 10; i++)
    m_Model->PerformClusteringIteration();
}

void SnakeWizardPanel::on_btnClusterReinitialize_clicked()
{
  m_Model->ReinitializeClustering();
}

void SnakeWizardPanel::on_btnClassifyTrain_clicked()
{
  try
  {
    m_Model->TrainClassifier();
  }
  catch (IRISException &exc)
  {
    ReportNonLethalException(this, exc, "Classification Failed");
  }
}

void SnakeWizardPanel::on_btnThreshDetail_clicked()
{
  m_SpeedDialog->ShowDialog();
}

void SnakeWizardPanel::on_btnClusterDetail_clicked()
{
  m_SpeedDialog->ShowDialog();
}

void SnakeWizardPanel::on_btnClassifyClearExamples_clicked()
{
  m_Model->ClearSegmentation();
}

void SnakeWizardPanel::onClassifyQuickLabelSelection()
{
  // Enter paintbrush mode - to help the user
  m_Model->GetParent()->GetGlobalState()->SetToolbarMode(PAINTBRUSH_MODE);
}

void SnakeWizardPanel::on_btnEdgeDetail_clicked()
{
  m_SpeedDialog->ShowDialog();
}
