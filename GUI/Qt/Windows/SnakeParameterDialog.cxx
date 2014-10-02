#include "SnakeParameterDialog.h"
#include "ui_SnakeParameterDialog.h"
#include "SnakeParameterModel.h"
#include "QtWidgetCoupling.h"
#include "QtCheckBoxCoupling.h"
#include "QtSliderCoupling.h"
#include "QtSpinBoxCoupling.h"
#include "QtDoubleSpinBoxCoupling.h"
#include "SnakeParameterPreviewRenderer.h"
#include "SnakeParametersPreviewPipeline.h"
#include "SNAPQtCommon.h"
#include <QTimer>
#include <QPushButton>

SnakeParameterDialog::SnakeParameterDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::SnakeParameterDialog)
{
  ui->setupUi(this);

  // Set up the preview renderers
  for(int i = 0; i < 4; i++)
    {
    m_PreviewRenderer[i] = SnakeParameterPreviewRenderer::New();
    m_PreviewRenderer[i]->SetForceToDisplay(
          static_cast<SnakeParameterPreviewRenderer::DisplayMode>(i));
    }

  ui->boxForceAlpha->SetRenderer(m_PreviewRenderer[0]);
  ui->boxForceBeta->SetRenderer(m_PreviewRenderer[1]);
  ui->boxForceGamma->SetRenderer(m_PreviewRenderer[2]);
  ui->boxForceTotal->SetRenderer(m_PreviewRenderer[3]);

  m_AnimationTimer = new QTimer(this);
  connect(m_AnimationTimer, SIGNAL(timeout()), SLOT(onAnimationTimer()));
}

SnakeParameterDialog::~SnakeParameterDialog()
{
  delete ui;
}

void SnakeParameterDialog::SetModel(SnakeParameterModel *model)
{
  this->m_Model = model;


  // Couple all of the spin boxes
  makeCoupling(ui->inZhuAlphaSimple, m_Model->GetWeightModel(SnakeParameterModel::ALHPA));
  makeCoupling(ui->inZhuAlphaSimpleSlider, m_Model->GetWeightModel(SnakeParameterModel::ALHPA));
  makeCoupling(ui->inZhuBetaSimple, m_Model->GetWeightModel(SnakeParameterModel::BETA));
  makeCoupling(ui->inZhuBetaSimpleSlider, m_Model->GetWeightModel(SnakeParameterModel::BETA));

  makeCoupling(ui->inCasellesAlphaSimple, m_Model->GetWeightModel(SnakeParameterModel::ALHPA));
  makeCoupling(ui->inCasellesAlphaSimpleSlider, m_Model->GetWeightModel(SnakeParameterModel::ALHPA));
  makeCoupling(ui->inCasellesBetaSimple, m_Model->GetWeightModel(SnakeParameterModel::BETA));
  makeCoupling(ui->inCasellesBetaSimpleSlider, m_Model->GetWeightModel(SnakeParameterModel::BETA));
  makeCoupling(ui->inCasellesGammaSimple, m_Model->GetWeightModel(SnakeParameterModel::GAMMA));
  makeCoupling(ui->inCasellesGammaSimpleSlider, m_Model->GetWeightModel(SnakeParameterModel::GAMMA));

  makeCoupling(ui->inAlphaMath, m_Model->GetWeightModel(SnakeParameterModel::ALHPA));
  makeCoupling(ui->inAlphaMathSlider, m_Model->GetWeightModel(SnakeParameterModel::ALHPA));
  makeCoupling(ui->inBetaMath, m_Model->GetWeightModel(SnakeParameterModel::BETA));
  makeCoupling(ui->inBetaMathSlider, m_Model->GetWeightModel(SnakeParameterModel::BETA));
  makeCoupling(ui->inGammaMath, m_Model->GetWeightModel(SnakeParameterModel::GAMMA));
  makeCoupling(ui->inGammaMathSlider, m_Model->GetWeightModel(SnakeParameterModel::GAMMA));
  makeCoupling(ui->inAlphaExp, m_Model->GetExponentModel(SnakeParameterModel::ALHPA));
  makeCoupling(ui->inBetaExp, m_Model->GetExponentModel(SnakeParameterModel::BETA));
  makeCoupling(ui->inGammaExp, m_Model->GetExponentModel(SnakeParameterModel::GAMMA));

  makeCoupling(ui->inSpeedup, m_Model->GetSpeedupFactorModel());
  makeCoupling(ui->inSpeedupSlider, m_Model->GetSpeedupFactorModel());

  // Couple the advanced checkbox
  makeCoupling(ui->chkAdvanced, m_Model->GetAdvancedEquationModeModel());

  // Couple the animation checkbutton to the corresponding model. In addition
  // register to receive events from the model, so we can toggle the timer that
  // does the actual animation
  makeCoupling(ui->chkAnimate, m_Model->GetAnimateDemoModel());
  LatentITKEventNotifier::connect(
        m_Model->GetAnimateDemoModel(), ValueChangedEvent(),
        this, SLOT(onModelUpdate(EventBucket)));

  // Fix the visibility of the widgets that depend on what snake mode we're in
  makeWidgetVisibilityCoupling(ui->inAlphaExp, m_Model->GetAdvancedEquationModeModel());
  makeWidgetVisibilityCoupling(ui->inBetaExp, m_Model->GetAdvancedEquationModeModel());
  makeWidgetVisibilityCoupling(ui->lblAlphaExp, m_Model->GetAdvancedEquationModeModel());
  makeWidgetVisibilityCoupling(ui->lblBetaExp, m_Model->GetAdvancedEquationModeModel());
  makeWidgetVisibilityCoupling(ui->inGammaExp, m_Model->GetAdvancedEquationModeModel());
  makeWidgetVisibilityCoupling(ui->lblGammaExp, m_Model->GetAdvancedEquationModeModel());

  makeWidgetVisibilityCoupling(ui->inGammaMath, m_Model->GetCasellesOrAdvancedModeModel());
  makeWidgetVisibilityCoupling(ui->inGammaMathSlider, m_Model->GetCasellesOrAdvancedModeModel());
  makeWidgetVisibilityCoupling(ui->lblGammaMath, m_Model->GetCasellesOrAdvancedModeModel());

  makeWidgetVisibilityCoupling(ui->boxForceGamma, m_Model->GetCasellesOrAdvancedModeModel());

  // Set up the preview renderers
  for(int i = 0; i < 4; i++)
    {
    m_PreviewRenderer[i]->SetModel(m_Model);
    }

  // React to parameter update events
  LatentITKEventNotifier::connect(m_Model, ModelUpdateEvent(),
                                  this, SLOT(onModelUpdate(const EventBucket &)));
}

void SnakeParameterDialog::onModelUpdate(const EventBucket &bucket)
{
  // Handle updates from the model
  if(bucket.HasEvent(ModelUpdateEvent()))
    {
    // Advanced mode
    bool advanced = m_Model->GetAdvancedEquationModeModel()->GetValue();

    // Update the state of the GUI not handled by the couplings
    if(m_Model->IsRegionSnake())
      {
      ui->stackIntuitive->setCurrentWidget(ui->pageZhu);
      if(!advanced)
        {
        ui->stackEqn->setCurrentWidget(ui->pageEqnZhu);
        }
      ui->lblForceAlpha->setText("Region Competition");
      }
    else
      {
      ui->stackIntuitive->setCurrentWidget(ui->pageCaselles);
      if(!advanced)
        {
        ui->stackEqn->setCurrentWidget(ui->pageEqnCaselles);
        }
      ui->lblForceAlpha->setText("Balloon Force");
      }

    // Update the formula
    if(advanced)
      {
      ui->stackEqn->setCurrentWidget(ui->pageEqnAdvanced);
      }
    }

  // Handle animation state changes
  if(bucket.HasEvent(ValueChangedEvent(), m_Model->GetAnimateDemoModel()))
    {
    bool animate = m_Model->GetAnimateDemo();
    if(animate && !m_AnimationTimer->isActive())
      {
      m_AnimationTimer->start(100);
      }
    else if(!animate && m_AnimationTimer->isActive())
      {
      m_Model->ResetAnimation();
      m_AnimationTimer->stop();
      }
    }
}

void SnakeParameterDialog::onAnimationTimer()
{
  m_Model->PerformAnimationStep();
}

void SnakeParameterDialog::on_btnRestore_clicked()
{
  m_Model->RestoreDefaults();
}

void SnakeParameterDialog::on_btnOpen_clicked()
{
  // Ask for a filename
  QString selection = ShowSimpleOpenDialogWithHistory(
        this, m_Model->GetParentModel(), "SnakeParameters",
        "Open Active Contour Evolution Parameters - ITK-SNAP",
        "Parameter File",
        "Text Files (*.txt)");

  // Open the labels from the selection
  if(selection.length())
    {
    try
      {
      m_Model->LoadParameters(to_utf8(selection));
      }
    catch(std::exception &exc)
      {
      ReportNonLethalException(this, exc, "Parameter File IO Error",
                               QString("Failed to load parameters"));
      }
    }
}

void SnakeParameterDialog::on_btnSave_clicked()
{
  // Ask for a filename
  QString selection = ShowSimpleSaveDialogWithHistory(
        this, m_Model->GetParentModel(), "SnakeParameters",
        "Save Active Contour Evolution Parameters - ITK-SNAP",
        "Parameter File",
        "Text Files (*.txt)",
        true);

  // Open the labels from the selection
  if(selection.length())
    {
    try
      {
      m_Model->SaveParameters(to_utf8(selection));
      }
    catch(std::exception &exc)
      {
      ReportNonLethalException(this, exc, "Parameter File IO Error",
                               QString("Failed to save parameters"));
      }
    }
}
