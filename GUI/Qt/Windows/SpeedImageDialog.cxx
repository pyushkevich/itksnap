#include "SpeedImageDialog.h"
#include "ui_SpeedImageDialog.h"
#include "QtDoubleSliderWithEditorCoupling.h"
#include "QtRadioButtonCoupling.h"
#include "SnakeWizardModel.h"
#include "QtWidgetActivator.h"
#include "ThresholdSettingsRenderer.h"
#include "QtCheckBoxCoupling.h"
#include <QCloseEvent>
#include <QShowEvent>

SpeedImageDialog::SpeedImageDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::SpeedImageDialog)
{
  ui->setupUi(this);

  // Create the renderer and attach it to its GL box
  m_ThresholdRenderer = ThresholdSettingsRenderer::New();
  ui->viewThreshold->SetRenderer(m_ThresholdRenderer);
}

SpeedImageDialog::~SpeedImageDialog()
{
  delete ui;
}

void SpeedImageDialog::SetModel(SnakeWizardModel *model)
{
  // Store the model
  m_Model = model;

  // Pass the model to the renderer
  m_ThresholdRenderer->SetModel(model);

  // Couple the widgets
  makeCoupling(ui->inLowerThreshold, model->GetThresholdLowerModel());
  makeCoupling(ui->inUpperThreshold, model->GetThresholdUpperModel());
  makeCoupling(ui->inThresholdSmoothness, model->GetThresholdSmoothnessModel());
  makeCoupling(ui->chkThresholdPreview, model->GetThresholdPreviewModel());

  makeRadioGroupCoupling(ui->grpThresholdMode, model->GetThresholdModeModel());

  // Set up activation
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

void SpeedImageDialog::showEvent(QShowEvent *event)
{
  // Same callback as when tabs change
  this->on_tabWidget_currentChanged(ui->tabWidget->currentIndex());
  event->accept();
}

void SpeedImageDialog::on_tabWidget_currentChanged(int index)
{
  if(ui->tabWidget->currentWidget() == ui->tabThreshold)
    {
    m_Model->OnThresholdingPageEnter();
    }
  else if(ui->tabWidget->currentWidget() == ui->tabEdge)
    {
    // m_Model->OnEdgePreprocessingPageEnter();
    }
}
