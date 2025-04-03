#include "PaintbrushToolPanel.h"
#include "DeepLearningSegmentationModel.h"
#include "ui_PaintbrushToolPanel.h"

#include "PaintbrushSettingsModel.h"
#include "MainImageWindow.h"
#include "SNAPQtCommon.h"
#include "PreferencesDialog.h"
#include "QtRadioButtonCoupling.h"
#include "QtCheckBoxCoupling.h"
#include "QtDoubleSpinBoxCoupling.h"
#include "QtSpinBoxCoupling.h"
#include "QtSliderCoupling.h"
#include "DeepLearningInfoDialog.h"
#include "DeepLearningSegmentationModel.h"
#include "GlobalUIModel.h"

/**
 * Traits for mapping status codes to a label
 */
class MiniConnectionStatusQLabelValueTraits
  : public WidgetValueTraitsBase<dls_model::ConnectionStatus, QLabel *>
{
public:
  typedef dls_model::ConnectionStatus TAtomic;

  virtual TAtomic GetValue(QLabel *w)
  {
    return dls_model::ConnectionStatus();
  }

  virtual void SetValue(QLabel *w, const TAtomic &value)
  {
    QString color;
    switch(value.status)
    {
      case dls_model::CONN_NO_SERVER:
        color = "darkred";
        break;
      case dls_model::CONN_CHECKING:
        color = "black";
        break;
      case dls_model::CONN_NOT_CONNECTED:
        color = "darkred";
        break;
      case dls_model::CONN_CONNECTED:
        color = "darkgreen";
        break;
    }
    w->setStyleSheet(QString("QLabel { color: %1; font-weight: bold; opacity: 0.5; }").arg(color));
  }
};

PaintbrushToolPanel::PaintbrushToolPanel(QWidget *parent) :
  SNAPComponent(parent),
  ui(new Ui::PaintbrushToolPanel)
{
  ui->setupUi(this);

  // Adjust the shortcuts for increase/decrease behavior
  ui->actionBrushIncrease->setShortcuts(
        ui->actionBrushIncrease->shortcuts() << QKeySequence('='));

  ui->actionBrushDecrease->setShortcuts(
        ui->actionBrushDecrease->shortcuts() << QKeySequence('_'));

  ui->actionGranularityIncrease->setShortcuts(
        ui->actionGranularityIncrease->shortcuts() << QKeySequence(Qt::META | Qt::Key_Equal) << QKeySequence(Qt::META | Qt::Key_Plus));

  ui->actionGranularityDecrease->setShortcuts(
        ui->actionGranularityDecrease->shortcuts() << QKeySequence(Qt::META | Qt::Key_Underscore) << QKeySequence(Qt::META | Qt::Key_Minus));

  ui->actionSmoothnessIncrease->setShortcuts(
        ui->actionSmoothnessIncrease->shortcuts() << QKeySequence(Qt::ALT | Qt::Key_Equal) << QKeySequence(Qt::ALT | Qt::Key_Plus));

  ui->actionSmoothnessDecrease->setShortcuts(
        ui->actionSmoothnessDecrease->shortcuts() << QKeySequence(Qt::ALT | Qt::Key_Underscore) << QKeySequence(Qt::ALT | Qt::Key_Minus));

  // Set up the info dialog
  m_DLInfoDialog = new DeepLearningInfoDialog(this);
  m_DLInfoDialog->setModal(true);
  QObject::connect(m_DLInfoDialog, &QDialog::finished,
                   this, &PaintbrushToolPanel::onDLInfoDialogFinished);

  addAction(ui->actionBrushIncrease);
  addAction(ui->actionBrushDecrease);
  addAction(ui->actionGranularityDecrease);
  addAction(ui->actionGranularityIncrease);
  addAction(ui->actionSmoothnessDecrease);
  addAction(ui->actionSmoothnessIncrease);
  addAction(ui->actionBrushStyle);
}

PaintbrushToolPanel::~PaintbrushToolPanel()
{
  delete ui;
}

void PaintbrushToolPanel::SetModel(PaintbrushSettingsModel *model)
{
  m_Model = model;

  // Couple the radio buttons
  std::map<PaintbrushShape, QAbstractButton *> rmap_shape{ { PAINTBRUSH_RECTANGULAR, ui->btnSquare },
                                                           { PAINTBRUSH_ROUND, ui->btnRound } };
  makeRadioGroupCoupling(ui->grpBrushShape, rmap_shape, m_Model->GetPaintbrushShapeModel());

  std::map<PaintbrushSmartMode, QAbstractButton *> rmap_smart_mode {
    { PAINTBRUSH_MANUAL, ui->btnManual },
    { PAINTBRUSH_WATERSHED, ui->btnWatershed },
    { PAINTBRUSH_DLS, ui->btnDeepLearning }
  };

  makeRadioGroupCoupling(ui->grpBrushMode, rmap_smart_mode, m_Model->GetPaintbrushSmartModeModel());

  // Couple the other controls
  makeCoupling(ui->chkVolumetric, model->GetVolumetricBrushModel());
  makeCoupling(ui->chkIsotropic, model->GetIsotropicBrushModel());
  makeCoupling(ui->chkChase, model->GetChaseCursorModel());

  makeCoupling(ui->inBrushSizeSlider, model->GetBrushSizeModel());
  makeCoupling(ui->inBrushSizeSpinbox, model->GetBrushSizeModel());

  // Couple the visibility of the adaptive widget
  makeWidgetVisibilityCoupling(ui->grpAdaptive, model->GetAdaptiveModeModel());
  makeWidgetVisibilityCoupling(ui->grpDeepLearning, model->GetDeepLearningModeModel());

  makeCoupling(ui->inGranularity, model->GetThresholdLevelModel());
  makeCoupling(ui->inSmoothness, model->GetSmoothingIterationsModel());

  activateOnFlag(ui->chkVolumetric, m_Model,
                 PaintbrushSettingsModel::UIF_VOLUMETRIC_OK);

  activateOnFlag(ui->btnWatershed, m_Model,
                 PaintbrushSettingsModel::UIF_ADAPTIVE_OK);

  // Listen to changes in smart mode, to display a popup
  connectITK(m_Model->GetPaintbrushSmartModeModel(), ValueChangedEvent());

  // Connect with the deep learning model
  auto *dls = m_Model->GetParentModel()->GetDeepLearningSegmentationModel();
  makeCoupling(ui->outDLStatus, dls->GetServerStatusModel(), MiniConnectionStatusQLabelValueTraits());
}

void
PaintbrushToolPanel::onModelUpdate(const EventBucket &bucket)
{
  if(bucket.HasEvent(ValueChangedEvent(), m_Model->GetPaintbrushSmartModeModel()))
  {
    if(m_Model->GetPaintbrushSmartModeModel()->GetValue() == PAINTBRUSH_DLS)
    {
      // Optionally, show the information dialog
      auto *dlm = m_Model->GetParentModel()->GetDeepLearningSegmentationModel();
      auto *ds = m_Model->GetParentModel()->GetGlobalDisplaySettings();
      if (dlm->GetServerStatus().status != dls_model::CONN_CONNECTED &&
          ds->GetFlagRemindDeepLearningExtensions())
      {
        m_DLInfoDialog->open();
      }
    }
  }
}

void PaintbrushToolPanel::on_actionBrushStyle_triggered()
{
  if(ui->btnSquare->isChecked())
    ui->btnRound->setChecked(true);
  else if(ui->btnRound->isChecked())
    ui->btnWatershed->setChecked(true);
  else if(ui->btnWatershed->isChecked())
    ui->btnSquare->setChecked(true);
}

void
PaintbrushToolPanel::on_btnConfigDL_clicked()
{
  MainImageWindow *winmain = findParentWidget<MainImageWindow>(this);
  winmain->GetPreferencesDialog()->ShowDialog();
  winmain->GetPreferencesDialog()->GoToPage(PreferencesDialog::DeepLearningServer);
}

void
PaintbrushToolPanel::onDLInfoDialogFinished(int result)
{
  if(result == QDialog::Accepted)
    on_btnConfigDL_clicked();
  if(m_DLInfoDialog->isDoNotShowAgainChecked())
  {
    m_Model->GetParentModel()
      ->GetGlobalDisplaySettings()
      ->GetFlagRemindDeepLearningExtensionsModel()
      ->SetValue(false);
  }
}
