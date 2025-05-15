#include "ContrastInspector.h"
#include "SNAPQtCommon.h"
#include "ui_ContrastInspector.h"
#include "QtStyles.h"
#include "IntensityCurveModel.h"
#include "QtCheckBoxCoupling.h"
#include "QtSpinBoxCoupling.h"
#include "QtDoubleSpinBoxCoupling.h"
#include "QtMenuCoupling.h"
#include "QtWidgetArrayCoupling.h"
#include "QtReporterDelegates.h"
#include "QtWidgetActivator.h"
#include "IntensityCurveVTKRenderer.h"

#include <QPalette>
#include <QDialog>
#include <QDialogButtonBox>

ContrastInspector::ContrastInspector(QWidget *parent) :
    SNAPComponent(parent),
    ui(new Ui::ContrastInspector)
{
  ui->setupUi(this);

  // Create the viewport reporter
  m_CurveBoxViewportReporter = QtViewportReporter::New();
  m_CurveBoxViewportReporter->SetClientWidget(ui->plotWidget);

  // Set up the renderer
  m_CurveRenderer = IntensityCurveVTKRenderer::New();
  ui->plotWidget->SetRenderer(m_CurveRenderer);
  m_CurveRenderer->SetBackgroundColor(Vector3d(1.0, 1.0, 1.0));

  // Create a menu of contrast actions
  QMenu *menu_actions = new QMenu(ui->btnContrastActions);
  m_ApplyToMenu = menu_actions->addMenu("Apply Contrast to Other Layers");
  QObject::connect(m_ApplyToMenu, SIGNAL(triggered(QAction*)), this, SLOT(onApplyToMenuTriggered(QAction*)));
  menu_actions->addSeparator();
  menu_actions->addAction(FindUpstreamAction(this, "actionResetContrastGlobal"));
  menu_actions->addAction(FindUpstreamAction(this, "actionAutoContrastGlobal"));
  menu_actions->addSeparator();
  menu_actions->addAction(ui->actionHistogram_display_preferences);
  ui->btnContrastActions->setMenu(menu_actions);

  // Hide the histogram options
  ui->grpHistogram->setVisible(false);
}

ContrastInspector::~ContrastInspector()
{
  delete ui;
}

void ContrastInspector::SetModel(IntensityCurveModel *model)
{
  // Set the model
  m_Model = model;

  // Set the model on the renderer
  m_CurveRenderer->SetModel(model);

  // Connect the viewport reporter to the model
  model->SetViewportReporter(m_CurveBoxViewportReporter);

  // Listen to model update events
  connectITK(m_Model, ModelUpdateEvent());

  // Set up the couplings. This is all we have to do to make the spin box
  // play with the model! There are no callbacks to write, no event handlers
  // to worry about! Yay!!!
  makeArrayCoupling(ui->inControlX, ui->inControlY, m_Model->GetMovingControlXYModel());

  // Set up couplings for window and level
  makeCoupling(ui->inMin, m_Model->GetIntensityRangeModel(IntensityCurveModel::MINIMUM));
  makeCoupling(ui->inMax, m_Model->GetIntensityRangeModel(IntensityCurveModel::MAXIMUM));
  makeCoupling(ui->inLevel, m_Model->GetIntensityRangeModel(IntensityCurveModel::LEVEL));
  makeCoupling(ui->inWindow, m_Model->GetIntensityRangeModel(IntensityCurveModel::WINDOW));

  // Make coupling for the control point id
  makeCoupling(ui->inControlId, m_Model->GetMovingControlIdModel());

  // Histogram bin controls
  makeCoupling(ui->inBinSize, m_Model->GetHistogramBinSizeModel());
  makeCoupling(ui->inCutoff, m_Model->GetHistogramCutoffModel());
  makeCoupling(ui->inLogScale, m_Model->GetHistogramScaleModel());

  // Couple visibility of the GL widget to the model having a layer
  makeWidgetVisibilityCoupling(ui->plotWidget, m_Model->GetHasLayerModel());

  // Handle activations
  activateOnFlag(this, m_Model,
                 IntensityCurveModel::UIF_LAYER_ACTIVE);
}

void ContrastInspector::on_btnRemoveControl_clicked()
{
  m_Model->OnControlPointNumberDecreaseAction();
}

void ContrastInspector::on_btnAddControl_clicked()
{
  m_Model->OnControlPointNumberIncreaseAction();
}

void ContrastInspector::on_btnReset_clicked()
{
  m_Model->OnResetCurveAction();
}

void ContrastInspector::UpdateApplyToMenu()
{
  // Update the 'Apply to' menu
  m_ApplyToMenu->clear();

  // Get the targets
  for(auto &t : m_Model->GetApplyToLayerTargets())
  {
    QAction *a = new QAction(m_ApplyToMenu);
    a->setText(QString::fromStdString(t.second));
    a->setData(QVariant::fromValue(t.first));
    m_ApplyToMenu->addAction(a);
    if(t.first.mode == ApplyToLayerSelection::APPLY_TO_ALL)
      m_ApplyToMenu->addSeparator();
  }

  // Toggle menu availability
  m_ApplyToMenu->setEnabled(m_Model->GetApplyToLayerTargets().size() > 0);
}

void ContrastInspector::onModelUpdate(const EventBucket &b)
{
  m_Model->Update();
  this->UpdateApplyToMenu();
}

void ContrastInspector::on_btnAuto_clicked()
{
  m_Model->OnAutoFitWindow();
}

void ContrastInspector::onApplyToMenuTriggered(QAction *a)
{
  m_Model->ApplyToLayers(a->data().value<ApplyToLayerSelection>());
}

void
ContrastInspector::on_actionHistogram_display_preferences_triggered()
{
  QDialog *dlg = new QDialog(this);
  QVBoxLayout *lo = new QVBoxLayout(dlg);
  QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok);
  lo->addWidget(ui->grpHistogram);
  lo->addWidget(box);
  connect(box, &QDialogButtonBox::accepted, dlg, &QDialog::accept);
  ui->grpHistogram->show();
  dlg->exec();
}
