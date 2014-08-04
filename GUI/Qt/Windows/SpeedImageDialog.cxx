#include "SpeedImageDialog.h"
#include "ui_SpeedImageDialog.h"
#include "QtSliderCoupling.h"
#include "QtRadioButtonCoupling.h"
#include "QtAbstractButtonCoupling.h"
#include "QtComboBoxCoupling.h"
#include "SnakeWizardModel.h"
#include "QtWidgetActivator.h"
#include "ThresholdSettingsRenderer.h"
#include "EdgePreprocessingSettingsRenderer.h"
#include "QtCheckBoxCoupling.h"
#include "QtSpinBoxCoupling.h"
#include "QtDoubleSpinBoxCoupling.h"
#include "QtComboBoxCoupling.h"
#include <QCloseEvent>
#include <QTreeView>
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "UnsupervisedClustering.h"
#include "GaussianMixtureModel.h"
#include "ImageWrapperBase.h"
#include "SNAPQtCommon.h"
#include "GMMTableModel.h"
#include "GMMRenderer.h"
#include "QtDoubleSliderWithEditorCoupling.h"
#include "QtPagedWidgetCoupling.h"

Q_DECLARE_METATYPE(SnakeWizardModel::LayerScalarRepIndex)

SpeedImageDialog::SpeedImageDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::SpeedImageDialog)
{
  ui->setupUi(this);

  // Set up the GMM table
  m_GMMTableModel = new GMMTableModel(this);
  ui->tblClusters->setModel(m_GMMTableModel);
  ui->tblClusters->setItemDelegate(new GMMItemDelegate(this));

  // Create the renderer and attach it to its GL box
  m_ThresholdRenderer = ThresholdSettingsRenderer::New();
  ui->viewThreshold->SetRenderer(m_ThresholdRenderer);

  // Same for the edge preprocessing
  m_EdgeSettingsRenderer = EdgePreprocessingSettingsRenderer::New();
  ui->viewEdgeMapping->SetRenderer(m_EdgeSettingsRenderer);

  m_GMMRenderer = GMMRenderer::New();
  ui->viewClustering->SetRenderer(m_GMMRenderer);
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
  m_GMMRenderer->SetModel(model);

  // Pass the model to the sub-models
  m_GMMTableModel->SetParentModel(model);

  // Couple the preview checkbox
  makeCoupling(ui->chkPreview, model->GetPreviewModel());
  makeCoupling((QAbstractButton *) ui->btnSpeedBlue, model->GetBlueWhiteSpeedModeModel());
  makeCoupling((QAbstractButton *) ui->btnSpeedOverlay, model->GetRedTransparentSpeedModeModel());

  // Couple the thresholding widgets
  makeCoupling(ui->inLowerThreshold, model->GetThresholdLowerModel());
  makeCoupling(ui->inLowerThresholdSpinner, model->GetThresholdLowerModel());
  makeCoupling(ui->inUpperThreshold, model->GetThresholdUpperModel());
  makeCoupling(ui->inUpperThresholdSpinner, model->GetThresholdUpperModel());
  makeCoupling(ui->inThresholdSmoothness, model->GetThresholdSmoothnessModel());
  makeCoupling(ui->inThresholdSmoothnessSpinner, model->GetThresholdSmoothnessModel());

  makeRadioGroupCoupling(ui->grpThresholdMode, model->GetThresholdModeModel());

  makeCoupling(ui->inThresholdImage, model->GetThresholdActiveLayerModel());
  makeCoupling(ui->inThresholdComponent, model->GetThresholdActiveScalarRepModel());

  // Couple the edge preprocessing widgets
  makeCoupling(ui->inEdgeSmoothing, model->GetEdgePreprocessingSigmaModel());
  makeCoupling(ui->inEdgeKappa, model->GetEdgePreprocessingKappaModel());
  makeCoupling(ui->inEdgeExponent, model->GetEdgePreprocessingExponentModel());

  // Couple the clustering widgets
  makeCoupling(ui->inNumClusters, model->GetNumberOfClustersModel());
  makeCoupling(ui->inNumSamples, model->GetNumberOfGMMSamplesModel());
  makeCoupling(ui->inClusterXComponent, model->GetClusterPlottedComponentModel());

  // Couple the tab pages to the current mode
  std::map<PreprocessingMode, QWidget *> preproc_page_map;
  preproc_page_map[PREPROCESS_THRESHOLD] = ui->tabThreshold;
  preproc_page_map[PREPROCESS_EDGE] = ui->tabEdgeAttraction;
  preproc_page_map[PREPROCESS_GMM] = ui->tabCluster;
  preproc_page_map[PREPROCESS_RF] = ui->tabClassify;
  makePagedWidgetCoupling(ui->tabMode, m_Model->GetPreprocessingModeModel(),
                          preproc_page_map);

  // Set up activation
  activateOnFlag(ui->inLowerThreshold, model, SnakeWizardModel::UIF_LOWER_THRESHOLD_ENABLED);
  activateOnFlag(ui->inLowerThresholdSpinner, model, SnakeWizardModel::UIF_LOWER_THRESHOLD_ENABLED);
  activateOnFlag(ui->inUpperThreshold, model, SnakeWizardModel::UIF_UPPER_THRESHOLD_ENABLED);
  activateOnFlag(ui->inUpperThresholdSpinner, model, SnakeWizardModel::UIF_UPPER_THRESHOLD_ENABLED);
}

void SpeedImageDialog::on_btnClose_clicked()
{
  this->accept();
}

void SpeedImageDialog::closeEvent(QCloseEvent *event)
{
  // Hide speed if there is no valid image
  this->on_btnClose_clicked();

  // Done with the event
  event->accept();
}

void SpeedImageDialog::ShowDialog()
{
  this->show();
  this->activateWindow();
  this->raise();
}

void SpeedImageDialog::on_btnReinitialize_clicked()
{
  m_Model->ReinitializeClustering();
}

void SpeedImageDialog::on_btnIterate_clicked()
{
  m_Model->PerformClusteringIteration();
}



void SpeedImageDialog::on_btnIterateTen_clicked()
{
  for(int i = 0; i < 10; i++)
    m_Model->PerformClusteringIteration();
}


void SpeedImageDialog::on_btnTrain_clicked()
{
  m_Model->TrainClassifier();
}
