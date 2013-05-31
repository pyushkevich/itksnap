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
#include "QtSpinBoxCoupling.h"
#include "IRISException.h"
#include <QMessageBox>
#include <QTimer>
#include <IRISApplication.h>

Q_DECLARE_METATYPE(SnakeType)

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
  std::vector<Bubble> &ba = m_Model->GetParent()->GetDriver()->GetBubbleArray();
  return ba.size();
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
}

SnakeWizardPanel::~SnakeWizardPanel()
{
  delete ui;
}

void SnakeWizardPanel::on_btnPreprocess_clicked()
{
  // Show the appropriately configured preprocessing dialog
  m_SpeedDialog->SetPageAndShow();
}


void SnakeWizardPanel::SetModel(GlobalUIModel *model)
{
  // Store and pass on the models
  m_ParentModel = model;
  m_Model = model->GetSnakeWizardModel();
  m_SpeedDialog->SetModel(m_Model);
  m_ParameterDialog->SetModel(model->GetSnakeParameterModel());

  // Couple widgets to models
  makeCoupling(ui->inSnakeType, m_Model->GetSnakeTypeModel());
  makeCoupling(ui->inBubbleRadius, m_Model->GetBubbleRadiusModel());

  /*
  // Couple the table to the active bubble model
  // TODO: simplify this garbage! this is too complex!
  typedef TextAndIconTableWidgetRowTraits<
      int, Bubble, BubbleItemDescriptionTraits> BubbleDomainRowTraits;

  typedef ItemSetWidgetDomainTraits<
      SnakeWizardModel::BubbleDomain,
      QTableWidget, BubbleDomainRowTraits> BubbleDomainTraits;

  typedef DefaultWidgetValueTraits<int, QTableWidget> BubbleValueTraits;

  makeCoupling(ui->tableBubbleList, m_Model->GetActiveBubbleModel(),
               BubbleValueTraits(), BubbleDomainTraits());
               */

  BubbleItemModel *biModel = new BubbleItemModel(this);
  biModel->setSourceModel(m_Model);
  ui->tableBubbleList->setModel(biModel);

  makeCoupling((QAbstractItemView *) ui->tableBubbleList,
               m_Model->GetActiveBubbleModel());

  makeCoupling(ui->inStepSize, m_Model->GetStepSizeModel());
  makeCoupling(ui->outIteration, m_Model->GetEvolutionIterationModel());

  // Activation flags
  activateOnNotFlag(ui->pgPreproc, m_Model,
                    SnakeWizardModel::UIF_PREPROCESSING_ACTIVE);

  activateOnFlag(ui->btnNextPreproc, m_Model,
                 SnakeWizardModel::UIF_SPEED_AVAILABLE);

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
  // Step the snake
  m_Model->PerformEvolutionStep();
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

  // Tell the model to return to initialization state
  m_Model->OnCancelSegmentation();

  // Tell parent to hide this window
  emit wizardFinished();
}
