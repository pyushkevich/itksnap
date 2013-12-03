#include "PreferencesDialog.h"
#include "ui_PreferencesDialog.h"
#include "GlobalPreferencesModel.h"
#include "MeshOptions.h"
#include "DefaultBehaviorSettings.h"

#include "QtCheckBoxCoupling.h"
#include "QtComboBoxCoupling.h"
#include "QtSpinBoxCoupling.h"
#include "QtDoubleSpinBoxCoupling.h"

PreferencesDialog::PreferencesDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::PreferencesDialog)
{
  ui->setupUi(this);
}

PreferencesDialog::~PreferencesDialog()
{
  delete ui;
}

void PreferencesDialog::SetModel(GlobalPreferencesModel *model)
{
  // Copy the model
  m_Model = model;

  // Hook up the default behavior settings
  DefaultBehaviorSettings *dbs = m_Model->GetDefaultBehaviorSettings();
  makeCoupling(ui->chkLinkedZoom, dbs->GetLinkedZoomModel());
  makeCoupling(ui->chkContinuousUpdate, dbs->GetContinuousMeshUpdateModel());
  makeCoupling(ui->chkSynchronize, dbs->GetSynchronizationModel());
  makeCoupling(ui->chkSyncCursor, dbs->GetSyncCursorModel());
  makeCoupling(ui->chkSyncZoom, dbs->GetSyncZoomModel());
  makeCoupling(ui->chkSyncPan, dbs->GetSyncPanModel());
  makeCoupling(ui->chkCheckForUpdates, dbs->GetCheckForUpdatesModel());

  // Hook up the mesh options
  MeshOptions *mo = m_Model->GetMeshOptions();

  makeCoupling(ui->chkGaussianSmooth, mo->GetUseGaussianSmoothingModel());
  makeCoupling(ui->inGaussianSmoothDeviation, mo->GetGaussianStandardDeviationModel());
  makeCoupling(ui->inGaussianSmoothMaxError, mo->GetGaussianErrorModel());

  makeCoupling(ui->chkMeshSmooth, mo->GetUseMeshSmoothingModel());
  makeCoupling(ui->inMeshSmoothConvergence, mo->GetMeshSmoothingConvergenceModel());
  makeCoupling(ui->inMeshSmoothFeatureAngle, mo->GetMeshSmoothingFeatureAngleModel());
  makeCoupling(ui->inMeshSmoothIterations, mo->GetMeshSmoothingIterationsModel());
  makeCoupling(ui->inMeshSmoothRelaxation, mo->GetMeshSmoothingRelaxationFactorModel());
  makeCoupling(ui->chkMeshSmoothBoundarySmoothing, mo->GetMeshSmoothingBoundarySmoothingModel());
  makeCoupling(ui->chkMeshSmoothFeatureEdgeSmoothing, mo->GetMeshSmoothingFeatureEdgeSmoothingModel());

  makeCoupling(ui->chkDecimate, mo->GetUseDecimationModel());
  makeCoupling(ui->inDecimateFeatureAngle, mo->GetDecimateFeatureAngleModel());
  makeCoupling(ui->inDecimateMaxError, mo->GetDecimateMaximumErrorModel());
  makeCoupling(ui->inDecimateTargetReduction, mo->GetDecimateTargetReductionModel());
  makeCoupling(ui->chkDecimatePreserveTopology, mo->GetDecimatePreserveTopologyModel());
}

void PreferencesDialog::show()
{
  if(!this->isVisible() && m_Model)
    {
    m_Model->InitializePreferences();
    }
  QDialog::show();
}
