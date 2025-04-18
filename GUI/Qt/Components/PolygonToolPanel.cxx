#include "PolygonToolPanel.h"
#include "MainImageWindow.h"
#include "PreferencesDialog.h"
#include "SNAPAppearanceSettings.h"
#include "SNAPQtCommon.h"
#include "ui_PolygonToolPanel.h"
#include "GlobalUIModel.h"

#include "QtCheckBoxCoupling.h"
#include "QtSpinBoxCoupling.h"
#include "QtSliderCoupling.h"
#include "QtRadioButtonCoupling.h"

#include "GlobalState.h"
#include "PolygonSettingsModel.h"
#include "QtWidgetActivator.h"
#include "DeepLearningConnectionStatusCouplingTraits.h"
#include "DeepLearningInfoDialog.h"

PolygonToolPanel::PolygonToolPanel(QWidget *parent) :
  SNAPComponent(parent),
  ui(new Ui::PolygonToolPanel)
{
  ui->setupUi(this);

  // Set up the info dialog
  m_DLInfoDialog = new DeepLearningInfoDialog(this);
  m_DLInfoDialog->setModal(true);
  QObject::connect(m_DLInfoDialog, &QDialog::finished,
                   this, &PolygonToolPanel::onDLInfoDialogFinished);
}

PolygonToolPanel::~PolygonToolPanel()
{
  delete ui;
}

void PolygonToolPanel::SetModel(GlobalUIModel *model)
{
  m_Model = model;

  // Couple the inversion checkbox
  makeCoupling(ui->chkInvertPolygon,
               m_Model->GetGlobalState()->GetPolygonInvertModel());

  // Couple the freehand drawing mode
  makeCoupling(ui->inSegmentLength,
               m_Model->GetPolygonSettingsModel()->GetFreehandSegmentLengthModel());
  makeCoupling(ui->inSegmentLengthSlider,
               m_Model->GetPolygonSettingsModel()->GetFreehandSegmentLengthModel());

  // Couple the deep learning option
  makeCoupling(ui->chkDeepLearningMode,
               m_Model->GetPolygonSettingsModel()->GetDeepLearningModeModel());

  // Couple the radio buttons
  std::map<bool, QAbstractButton *> radioMap;
  radioMap[false] = ui->btnSmooth;
  radioMap[true] = ui->btnPiecewiseLinear;
  makeRadioGroupCoupling(ui->grpCurveStyle, radioMap,
                         m_Model->GetPolygonSettingsModel()->GetFreehandIsPiecewiseModel());

  // Toggle the appearance of the piecewise block
  makeWidgetVisibilityCoupling(ui->grpSegmentLength,
                               m_Model->GetPolygonSettingsModel()->GetFreehandIsPiecewiseModel());

  auto *dlmm = m_Model->GetPolygonSettingsModel()->GetDeepLearningModeModel();
  makeWidgetVisibilityCoupling(ui->grpDeepLearning, dlmm);

  // Listen to changes in smart/AI mode, to display a popup
  connectITK(dlmm, ValueChangedEvent());

  // Connect with the deep learning model
  auto *dls = m_Model->GetDeepLearningSegmentationModel();
  makeCoupling(ui->outDLStatus, dls->GetServerStatusModel(), MiniConnectionStatusQLabelValueTraits());
}

void
PolygonToolPanel::onModelUpdate(const EventBucket &bucket)
{
  auto *dlmm = m_Model->GetPolygonSettingsModel()->GetDeepLearningModeModel();
  if (bucket.HasEvent(ValueChangedEvent(), dlmm))
  {
    if(dlmm->GetValue())
    {
      // Optionally, show the information dialog
      auto *dlm = m_Model->GetDeepLearningSegmentationModel();
      auto *ds = m_Model->GetGlobalDisplaySettings();
      if (dlm->GetServerStatus().status != dls_model::CONN_CONNECTED &&
          ds->GetFlagRemindDeepLearningExtensions())
      {
        m_DLInfoDialog->open();
      }
    }
  }
}

void
PolygonToolPanel::on_btnConfigDL_clicked()
{
  MainImageWindow *winmain = findParentWidget<MainImageWindow>(this);
  winmain->GetPreferencesDialog()->ShowDialog();
  winmain->GetPreferencesDialog()->GoToPage(PreferencesDialog::DeepLearningServer);
}

void
PolygonToolPanel::onDLInfoDialogFinished(int result)
{
  if (result == QDialog::Accepted)
    on_btnConfigDL_clicked();
  if (m_DLInfoDialog->isDoNotShowAgainChecked())
  {
    m_Model->GetGlobalDisplaySettings()->GetFlagRemindDeepLearningExtensionsModel()->SetValue(false);
  }
}
