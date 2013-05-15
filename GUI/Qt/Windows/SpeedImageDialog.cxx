#include "SpeedImageDialog.h"
#include "ui_SpeedImageDialog.h"
#include "QtDoubleSliderWithEditorCoupling.h"
#include "QtRadioButtonCoupling.h"
#include "SnakeWizardModel.h"
#include "QtWidgetActivator.h"
#include "ThresholdSettingsRenderer.h"
#include "EdgePreprocessingSettingsRenderer.h"
#include "QtCheckBoxCoupling.h"
#include "QtSpinBoxCoupling.h"
#include <QCloseEvent>




SpeedImageDialog::SpeedImageDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::SpeedImageDialog)
{
  ui->setupUi(this);

  // Set up the GMM table
  m_GMMTableModel = new GMMTableModel(this);
  ui->tblClusters->setModel(m_GMMTableModel);

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

  // Pass the model to the sub-models
  m_GMMTableModel->SetParentModel(model);

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

  // Couple the clustering widgets
  makeCoupling(ui->inNumClusters, model->GetNumberOfClustersModel());

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
  else if(ui->tabWidgetInOut->currentWidget() == ui->tabCluster)
    {
    m_Model->OnClusteringPageEnter();
    }
}

void SpeedImageDialog::on_btnReinitialize_clicked()
{
  m_Model->ReinitializeClustering();
}

void SpeedImageDialog::on_btnIterate_clicked()
{
  m_Model->PerformClusteringIteration();
}














#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "UnsupervisedClustering.h"
#include "GaussianMixtureModel.h"

void GMMTableModel::SetParentModel(SnakeWizardModel *parent)
{
  m_ParentModel = parent;

  LatentITKEventNotifier::connect(m_ParentModel,
                                  SnakeWizardModel::GMMModifiedEvent(),
                                  this, SLOT(onMixtureModelChange()));
}

void GMMTableModel::onMixtureModelChange()
{
  this->layoutChanged();
}

GaussianMixtureModel *GMMTableModel::GetGMM() const
{
  if(!m_ParentModel)
    return NULL;

  // Get the unsupervised clustering class
  UnsupervisedClustering *uc =
    m_ParentModel->GetParent()->GetDriver()->GetClusteringEngine();

  // If we are not in GMM mode, there is no uc!
  if(!uc) return NULL;

  // Get the number of clusters
  return uc->GetMixtureModel();
}

int GMMTableModel::rowCount(const QModelIndex &parent) const
{
  // Get the number of clusters
  GaussianMixtureModel *gmm = this->GetGMM();
  return gmm ? gmm->GetNumberOfGaussians() : 0;
}

int GMMTableModel::columnCount(const QModelIndex &parent) const
{
  // Get the number of clusters
  GaussianMixtureModel *gmm = this->GetGMM();
  return gmm ? gmm->GetNumberOfComponents() + 2 : 0;
}

QVariant GMMTableModel::data(const QModelIndex &index, int role) const
{
  // Only return value for display role
  if(role == Qt::CheckStateRole && index.column() == 0)
    {
    return Qt::Unchecked;
    }

  else if(role != Qt::DisplayRole)
    return QVariant();

  // Get the number of clusters
  GaussianMixtureModel *gmm = this->GetGMM();
  assert(gmm);

  // Get the proper component
  int cluster = index.row();
  if(index.column() == 0)
    return QVariant();
  if(index.column() == 1)
    return gmm->GetWeight(cluster);
  else
    return gmm->GetMean(cluster)[index.column()-2];

  return QVariant();
}

QVariant GMMTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if(role != Qt::DisplayRole)
    return QVariant();

  if(orientation == Qt::Horizontal)
    {
    if(section == 0)
      {
      return "Primary";
      }
    else if(section == 1)
      {
      return "Weight";
      }
    else
      {
      return QString("Comp. %1").arg(section-1);
      }
    }

  else if(orientation == Qt::Vertical)
    {
    return QString("Cluster %1").arg(section+1);
    }

  return QVariant();
}


GMMTableModel::GMMTableModel(QObject *parent)
  : QAbstractTableModel(parent)
{
  m_ParentModel = NULL;
}

