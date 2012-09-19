#include "SpeedImageDialog.h"
#include "ui_SpeedImageDialog.h"
#include "QtDoubleSliderWithEditorCoupling.h"
#include "QtRadioButtonCoupling.h"
#include "SnakeWizardModel.h"
#include "QtWidgetActivator.h"
#include "ThresholdSettingsRenderer.h"
#include "EdgePreprocessingSettingsRenderer.h"
#include "QtCheckBoxCoupling.h"
#include <QCloseEvent>

SpeedImageDialog::SpeedImageDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::SpeedImageDialog)
{
  ui->setupUi(this);

  // Create the renderer and attach it to its GL box
  m_ThresholdRenderer = ThresholdSettingsRenderer::New();
  ui->viewThreshold->SetRenderer(m_ThresholdRenderer);

  // Same for the edge preprocessing
  m_EdgeSettingsRenderer = EdgePreprocessingSettingsRenderer::New();
  ui->viewEdgeMapping->SetRenderer(m_EdgeSettingsRenderer);
}

SpeedImageDialog::~SpeedImageDialog()
{
  delete ui;
}

void SpeedImageDialog::SetModel(SnakeWizardModel *model)
{
  // Store the model
  m_Model = model;

  // Pass the model to the renderers
  m_ThresholdRenderer->SetModel(model);
  m_EdgeSettingsRenderer->SetModel(model);


  // Couple the thresholding widgets
  makeCoupling(ui->inLowerThreshold, model->GetThresholdLowerModel());
  makeCoupling(ui->inUpperThreshold, model->GetThresholdUpperModel());
  makeCoupling(ui->inThresholdSmoothness, model->GetThresholdSmoothnessModel());
  makeCoupling(ui->chkThresholdPreview, model->GetThresholdPreviewModel());

  makeRadioGroupCoupling(ui->grpThresholdMode, model->GetThresholdModeModel());

  // Couple the edge preprocessing widgets
  makeCoupling(ui->inEdgeSmoothing, model->GetEdgePreprocessingSigmaModel());
  makeCoupling(ui->inEdgeKappa, model->GetEdgePreprocessingKappaModel());
  makeCoupling(ui->inEdgeExponent, model->GetEdgePreprocessingExponentModel());
  makeCoupling(ui->chkEdgePreview, model->GetEdgePreprocessingPreviewModel());

  // Set up activation
  activateOnFlag(ui->tabThreshold, model, SnakeWizardModel::UIF_THESHOLDING_ENABLED);
  activateOnFlag(ui->tabEdge, model, SnakeWizardModel::UIF_EDGEPROCESSING_ENABLED);
  activateOnFlag(ui->inLowerThreshold, model, SnakeWizardModel::UIF_LOWER_THRESHOLD_ENABLED);
  activateOnFlag(ui->inUpperThreshold, model, SnakeWizardModel::UIF_UPPER_THRESHOLD_ENABLED);
}

void SpeedImageDialog::on_btnApply_clicked()
{
  // TODO: this should differ based on the selected page
  m_Model->ApplyThresholdPreprocessing();
}

void SpeedImageDialog::on_btnOk_clicked()
{
  m_Model->ApplyThresholdPreprocessing();
  m_Model->OnPreprocessingDialogClose();
  this->accept();
}

void SpeedImageDialog::on_btnClose_clicked()
{
  m_Model->OnPreprocessingDialogClose();
  this->reject();
}

void SpeedImageDialog::closeEvent(QCloseEvent *event)
{
  // Hide speed if there is no valid image
  this->on_btnClose_clicked();

  // Done with the event
  event->accept();
}

void SpeedImageDialog::SetPageAndShow()
{
  // Select appropriate tab?
  if(m_Model->GetSnakeTypeModel()->GetValue() == IN_OUT_SNAKE)
    {
    ui->stack->setCurrentWidget(ui->pgInOut);
    this->on_tabWidgetInOut_currentChanged(ui->tabWidgetInOut->currentIndex());
    }
  else if(m_Model->GetSnakeTypeModel()->GetValue() == EDGE_SNAKE)
    {
    ui->stack->setCurrentWidget(ui->pgEdge);
    this->on_tabWidgetEdge_currentChanged(ui->tabWidgetEdge->currentIndex());
    }

  this->show();
}

void SpeedImageDialog::on_tabWidgetEdge_currentChanged(int index)
{
  if(ui->tabWidgetEdge->currentWidget() == ui->tabEdge)
    {
    m_Model->OnEdgePreprocessingPageEnter();
    }
}

void SpeedImageDialog::on_tabWidgetInOut_currentChanged(int index)
{
  if(ui->tabWidgetInOut->currentWidget() == ui->tabThreshold)
    {
    m_Model->OnThresholdingPageEnter();
    }
}
