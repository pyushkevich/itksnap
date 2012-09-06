#include "SnakeWizardPanel.h"
#include "ui_SnakeWizardPanel.h"
#include "SpeedImageDialog.h"
#include "GlobalUIModel.h"
#include "SnakeWizardModel.h"
#include "QtComboBoxCoupling.h"
#include "QtWidgetActivator.h"
#include "QtTableWidgetCoupling.h"
#include "QtDoubleSliderWithEditorCoupling.h"
#include "QtSpinBoxCoupling.h"
#include "IRISException.h"
#include <QMessageBox>
#include <QTimer>

Q_DECLARE_METATYPE(SnakeType)


/**
  A traits class for mapping bubble structures into Qt table rows
  */
class BubbleItemDescriptionTraits
{
public:
  static QString GetText(int value, const Bubble &desc, int column)
  {
    if(column < 3)
      {
      return QString("%1").arg(desc.center[column]);
      }
    else if(column == 3)
      {
      return QString("%1").arg(desc.radius);
      }
    else return QString();
  }

  static QIcon GetIcon(int value, const Bubble &desc, int column)
  {
    return QIcon();
  }

  static QVariant GetIconSignature(int value, const Bubble &desc, int column)
  {
    return QVariant();
  }
};


SnakeWizardPanel::SnakeWizardPanel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SnakeWizardPanel)
{
  ui->setupUi(this);

  m_SpeedDialog = new SpeedImageDialog(this);

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
  m_SpeedDialog->show();
}

void SnakeWizardPanel::SetModel(GlobalUIModel *model)
{
  // Store and pass on the models
  m_ParentModel = model;
  m_Model = model->GetSnakeWizardModel();
  m_SpeedDialog->SetModel(m_Model);

  // Couple widgets to models
  makeCoupling(ui->inSnakeType, m_Model->GetSnakeTypeModel());
  makeCoupling(ui->inBubbleRadius, m_Model->GetBubbleRadiusModel());

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

void SnakeWizardPanel::on_btnPlay_clicked(bool checked)
{
  // This is where we toggle the snake evolution!
  if(checked)
    m_EvolutionTimer->start(100);
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
  m_Model->PerformEvolutionStep();
}
